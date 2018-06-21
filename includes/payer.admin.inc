<?php

/**
 * @file
 * Provides the Payer administrative interface.
 */

/**
 * Form of settings where you can change the URL of the purse replenishment page
 */
function payer_settings($form) {
  $form['payer_payment_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Payment url'),
    '#description' => t('Use a relative path and don\'t add a trailing slash or the URL won\'t work.'),
    '#default_value' => variable_get('payer_payment_url'),
    '#required' => TRUE,
    '#maxlength' => 255,
  );
  $form = system_settings_form($form);
  array_unshift($form['#submit'], 'payer_settings_submit');

  return $form;
}

/**
 * Module configuration form handler
 */
function payer_settings_submit($form, &$form_state) {
  $payment_url = variable_get('payer_payment_url');
  // Если любой из url отличается, перестраиваем меню.
  if($form_state['values']['payer_payment_url'] != $payment_url) {
    menu_rebuild();
    drupal_set_message(t('The menu has been rebuilt.'));
  }
}

/**
 * Applies filters for transactions administration filters based on session.
 *
 * @param $query
 *   A SelectQuery to which the filters should be applied.
 */
function payer_build_filter_transactions_query(SelectQueryInterface $query) {
  // Build query
  $session = isset($_SESSION['payer_transaction_overview_filter']) ? $_SESSION['payer_transaction_overview_filter'] : array();

  if(isset($session['username'])) {
    $user = user_load_by_name($session['username']);
    if($user != FALSE) {
      $query->condition('t.uid', $user->uid);
    }
  }

  if(isset($session['act'])) {
    $query->condition('t.act', $session['act']);
  }

  if(isset($session['status'])) {
    $query->condition('t.status', $session['status']);
  }
}

/**
 * Output of all available transactions with sorting capability
 */
function payer_transactions_list() {
  $header = array(
    array('data' => 'ID', 'field' => 'tid'),
    array('data' => 'User', 'field' => 'uid'),
    array('data' => 'Created', 'field' => 'created'),
    array('data' => 'Amount', 'field' => 'amount'),
    array('data' => 'Act', 'field' => 'act'),
    array('data' => 'Status', 'field' => 'status'),
    array('data' => 'Method', 'field' => 'method'),
    array('data' => 'Info', 'field' => 'info'),
  );

  // Generate query to database
  $query = db_select('payer_transactions', 't');
  $query->fields('t',
      array(
        'tid',
        'uid',
        'created',
        'amount',
        'act',
        'status',
        'method',
        'info',
      )
    )
    ->extend('PagerDefault')
    ->limit(25)
    ->extend('TableSort')
    ->orderByHeader($header);
  payer_build_filter_transactions_query($query);
  $transactions = $query->execute();

  $rows = array();
  foreach ($transactions as $transaction) {
    $user = user_load($transaction->uid);
    $user_link = l($user->name, url('user/'.$user->uid));

    $rows[] = array(
      l($transaction->tid, url('admin/config/payer/transactions/'.$transaction->tid)),
      $user_link,
      format_date($transaction->created, $type = 'short'),
      array(
        'data'  => $transaction->amount,
        'class' => array('transaction-amount', 'transaction-amount-'.$transaction->act)
      ),
      $transaction->act ? 'Coming':'Spending',
      $transaction->status ? 'Completed':'Waiting',
      $transaction->method,
      $transaction->info,
    );
  }
  $filter_form = drupal_get_form('payer_transactions_list_filter_form');

  $page_data = drupal_render($filter_form);
  $page_data .= theme('table', array('header' => $header, 'rows' => $rows));
  $page_data .= theme('pager');


  return theme(
    'payer_template_transactions_list',
    array('page_data' => $page_data)
  );
}

function payer_transactions_list_filter_form($form) {
  $session = isset($_SESSION['payer_transaction_overview_filter']) ? $_SESSION['payer_transaction_overview_filter'] : array();

  $values = array();

  foreach ($session as $key => $value) {
    $values[$key] = $value;
  }

  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Show only items where'),
    '#weight' => 0,
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#tree' => FALSE,
    '#attributes' => array(
      'class' => array('clearfix')
    ),
  );

  $form['filters']['items'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('clearfix')),
  );

  $form['filters']['items']['filters'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('exposed-filters')),
  );

  $form['filters']['items']['filters']['username'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#autocomplete_path' => 'user/autocomplete',
    '#size' => '23',
    '#default_value' => isset($values['username']) ? $values['username'] : '',
    '#maxlength' => '60',
  );

  $form['filters']['items']['filters']['act'] = array(
    '#type' => 'select',
    '#title' => t('Act'),
    '#default_value' => isset($values['act']) ? $values['act'] : '',
    '#options' => array(
      'any' => '- Any -',
      0 => 'Spending',
      1 => 'Coming',
    ),
  );

  $form['filters']['items']['filters']['status'] = array(
    '#type' => 'select',
    '#title' => t('Status'),
    '#default_value' => isset($values['status'])  ? $values['status'] : '',
    '#options' => array(
      'any' => '- Any -',
      0 => 'Waiting',
      1 => 'Completed',
    ),
  );

  $form['filters']['items']['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
  );

  if(count($session)) {
    $form['filters']['items']['actions']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
    );
  }
  $form['#submit'][] = 'payer_transactions_list_filter_form_submit';

  return $form;
}

function payer_transactions_list_filter_form_submit($form, &$form_state) {
  if($form_state['values']['op'] == t("Reset")) {
    $_SESSION['payer_transaction_overview_filter'] = array();
  }
  else {
    if(!empty($form_state['values']['username'])) {
      $_SESSION['payer_transaction_overview_filter']['username'] = $form_state['values']['username'];
    }
    else {
      unset($_SESSION['payer_transaction_overview_filter']['username']);
    }
    if($form_state['values']['act'] != 'any') {
      $_SESSION['payer_transaction_overview_filter']['act'] = $form_state['values']['act'];
    }
    else {
      unset($_SESSION['payer_transaction_overview_filter']['act']);
    }
    if($form_state['values']['status'] != 'any') {
      $_SESSION['payer_transaction_overview_filter']['status'] = $form_state['values']['status'];
    }
    else {
      unset($_SESSION['payer_transaction_overview_filter']['status']);
    }
  }
}

/**
 * Description of the form of adding a transaction
 */
function payer_add_transaction($form) {
  $form['payer_transaction_amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Amount'),
    '#element_validate' => array('element_validate_number'),
    '#required' => TRUE,
    '#maxlength' => 20,
    '#size' => 20,
  );

  $form['payer_transaction_act'] = array(
    '#type' => 'radios',
    '#title' => t('Act'),
    '#options' => array(
      0 => 'Spending',
      1 => 'Coming',
    ),
    '#required' => TRUE,
  );

  $form['payer_transaction_status'] = array(
    '#type' => 'radios',
    '#title' => t('Status'),
    '#options' => array(
      0 => 'Waiting',
      1 => 'Completed',
    ),
    '#required' => TRUE,
  );

  $form['payer_transaction_method'] = array(
    '#type' => 'textfield',
    '#title' => t('Method'),
    '#maxlength' => 255,
    '#default_value' => t('Manually'),
  );

  $form['payer_transaction_info'] = array(
    '#type' => 'textfield',
    '#title' => t('Info'),
    '#maxlength' => 255,
  );

  $form['payer_transaction_username'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#description' => t('Leave blank for the current user'),
    '#default_value' => '',
    '#autocomplete_path' => 'user/autocomplete',
    '#size' => '20',
    '#maxlength' => '60',
  );

  $form['actions']['custom_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create transaction'),
  );

  $form['#validate'] = array('payer_add_transaction_validate');
  $form['#submit'] = array('payer_add_transaction_submit');

  return $form;
}

/**
 * Validation of the form of adding a transaction
 */
function payer_add_transaction_validate($form, &$form_state) {
  $act = $form_state['values']['payer_transaction_act'];
  $amount = $form_state['values']['payer_transaction_amount'];

  $user = user_load_by_name($form_state['values']['payer_transaction_username']);

  if (empty($user)) {
    form_set_error('payer_transaction_username', t('User not found.'));
  }

  if(!empty($amount)) {
    // Class PayerTransaction is in the file "includes/payer.transaction.inc".
    $balance = PayerTransaction::get_user_balance($user);
    $sum_balance = PayerTransaction::calc_sum_balance($act, $balance, $amount);

    if (isset($amount) && $act == 0 && $sum_balance < 0) {
      form_set_error('payer_transaction_amount', t('Insufficient funds!'));
    }
  }
}

/**
 * Submit handler of the form of adding a transaction
 */
function payer_add_transaction_submit($form, &$form_state) {
  $username = $form_state['values']['payer_transaction_username'];

  if(!empty($username)) {
    $user = user_load_by_name($form_state['values']['payer_transaction_username']);
  }
  else {
    global $user;
  }

  $vars = array();
  $vars['uid'] = $user->uid;
  $vars['status'] = $form_state['values']['payer_transaction_status'];
  $vars['act'] = $form_state['values']['payer_transaction_act'];

  $vars['amount'] = $form_state['values']['payer_transaction_amount'];
  $vars['amount'] = number_format($vars['amount'], 2,'.', '');

  $vars['method'] = $form_state['values']['payer_transaction_method'];
  $vars['info'] = $form_state['values']['payer_transaction_info'];
  $vars['created'] = REQUEST_TIME;

  $transaction = new PayerTransaction($vars);
  $transaction->transaction();
}

/**
 * Handler of the pages of the list of methods of the module Payer
 **/
function payer_methods_list() {
  $existing_methods = _get_methods();
  // Table header with content
  $headers = array(
    array('class' => array('pay-method_status')),
    array('data' => 'Method'),
    array('data' => 'Operations'),
  );

  // Table rows with content
  $rows = array();

  /**
   * Preparation of information on cash payment methods
   * in the folder "payer/methods"
   */
  foreach($existing_methods as $pay_method) {
    $options = array(
      'config_link' => NULL, // ссылка на конфиг метода оплаты.
      'method_name' => NULL, // системное название метода.
      'description' => NULL, // описание метода.
      'image' => NULL, // изображение.
      'info' => NULL, // информация, содержащаяся в файле .info
      'status' => FALSE, // Флаг активности модуля
    );

    $options['info'] = drupal_parse_info_file($pay_method->uri);
    $options['method_name'] = $pay_method->name;
    $options['status'] = module_exists($options['method_name']) ? TRUE : FALSE;

    if(!$options['status']) {
      $options['description'] .= '<div class="messages warning">';
      $options['description'] .= t(
        'The payment method &quot;@pay_method&quot; is installed, but it is off. To use the module functionality, enable it on the !page_modules under &quot;Payment&quot;',
        array(
          '@pay_method' => $options['info']['name'],
          '!page_modules' => l(t('modules page'), 'admin/modules'),
        )
      );
      $options['description'] .= '</div>';
      $options['status'] = '🚫';
    }
    else {
      $options['config_link'] = l('Config', $options['info']['configure']);
      $options['status'] = '✅';
    }

    $options['image'] = drupal_get_path('module', $options['method_name']) . '/preview.png';

    if(file_exists($options['image'])) {
      $options['image'] = theme('image', array('path' => $options['image']));
    }
    else {
      $options['image'] = NULL;
    }

    $options['description'] .= '<h3>' . $options['info']['name'] . '</h3>';
    $options['description'] .= theme('default', array());;
    $options['description'] .= $options['image'];
    $options['description'] .= '<p>' . $options['info']['description'] . '</p>';

    // Добавление вычесленных данных в массив $rows
    $rows[$options['method_name']] = array(
      'data' => array(
        array(
          'data'  => $options['status'],
          'class' => array('pay-method_status')
        ),
        $options['description'],
        $options['config_link'],
      ),
    );
  }

  usort($rows, function($a, $b){
    if($a['data'][0]['data'] === $b['data'][0]['data'])
      return 0;

    return $a['data'][0]['data'] > $b['data'][0]['data'] ? 1 : -1;
  });

  // Assembling data for a page
  $page_data = array(
    'header' => $headers,
    'rows' => $rows,
    'sticky' => TRUE,
    'empty' => t('No results found'),
    'attributes' => array(),
    'caption' => FALSE,
    'colgroups' => array(),
  );

  return theme(
    'payer_template_methods_list',
    array('page_data' => $page_data)
  );
}