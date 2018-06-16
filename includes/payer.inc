<?php

class Payer {
  /**
   * Объект пользователя Drupal
   **/
  private $user;

  /**
   * ID события оплаты, title материала
   */
  public $pay_id = NULL;

  /**
   * Количество денег на счету пользователя поле "payer_wallet"
   **/
  private $score = NULL;

  /**
   * Действие с кошельком пользователя:
   *  0 - Расход (списание)
   *  1 - Приход (пополнение)
   **/
  private $act = NULL;

  /**
   * Действие с кошельком пользователя:
   *  0 - Ожидание (счет создан и ожидает оплаты)
   *  1 - Исполнен (счет исполнен)
   **/
  private $status = NULL;

  /**
   * Сумма которую необходимо списать или пополнить
   * кошелек пользователя
   **/
  private $amount = NULL;

  /**
   * Сколько денег останется после действий с кошельком:
   *   а) при расходе (0) - разность $wallet и $amount
   *   б) при приходе (1) - сумма $wallet и $amount
   * кошелек пользователя
   **/
  private $balance = NULL;

  /**
   * Комментарий к платежу
   **/
  private $comment = NULL;

  public function __construct($vars) {
    if (isset($vars['uid'])) {
      $this->user = self::get_user_object($vars['uid']);
      $this->score = self::get_user_balance($this->user);
    }
    if (isset($vars['pay_id'])){
      $this->pay_id = $vars['pay_id'];
    }
    if (isset($vars['act'])){
      $this->act = $vars['act'];
    }
    if (isset($vars['status'])){
      $this->status = $vars['status'];
    }
    if (isset($vars['amount'])){
      $this->amount = number_format($vars['amount'], 2,'.', '');
    }
    if (isset($vars['comment'])){
      $this->comment = $vars['comment'];
    }
  }

  /**
   * Получить объект пользователя по uid
   **/
  static public function get_user_object($uid = NULL) {
    // Если uid не был передан загружаем текущего пользователя
    if (!isset($uid)){
      global $user;
      $uid = $user->uid;
    }
    return user_load($uid);
  }

  /**
   * Получить статус материала
   */
  static public function get_event_pay_status($vars) {
    if(!isset($vars['node'])){
      if(!isset($vars['nid'])) {
        $vars['nid'] = self::get_event_pay_nid($vars['pay_id']);
      }

      $status = db_select('field_data_events_pay_status', 's')
        ->fields('s', array('events_pay_status_value'))
        ->condition('s.entity_id', $vars['nid'])
        ->execute()
        ->fetchAssoc();

      $status = $status['events_pay_status_value'];
    }
    else {
      $status = field_get_items( 'node' , $vars['node'], 'events_pay_status' );
      $status = $status[0]['value'];
    }
    return $status;
  }

  /**
   * Получить сумму счета
   *
   * @param $vars
   *
   * @return mixed
   */
  static public function get_event_pay_amount($vars) {
    if(!isset($vars['node'])){
      if(!isset($vars['nid'])) {
        $vars['nid'] = self::get_event_pay_nid($vars['pay_id']);
      }

      $amount = db_select('field_data_events_pay_amount', 's')
        ->fields('s', array('events_pay_amount_value'))
        ->condition('s.entity_id', $vars['nid'])
        ->execute()
        ->fetchAssoc();

      $amount = $amount['events_pay_amount_value'];
    }
    else {
      $amount = field_get_items( 'node' , $vars['node'], 'events_pay_amount' );
      $amount = $amount[0]['value'];
    }
    return $amount;
  }

  /**
   * @param $pay_id
   * @param null $nid
   *
   * @return bool|mixed
   */
  static public function get_event_pay_node($pay_id, $nid = NULL) {
    if(!isset($nid)) {
      $nid = self::get_event_pay_nid($pay_id);
    }

    return node_load($nid);
  }

  /**
   * @param $pay_id
   *
   * @return int — nid
   */
  static public function get_event_pay_nid($pay_id) {
    $node = db_select('node', 'n')
      ->fields('n', array('nid'))
      ->condition('n.type', 'events_pay')
      ->condition('n.title', $pay_id)
      ->execute()
      ->fetchAssoc();

    return $node['nid'];
  }

  /**
   * Получение количества материалов типа "events_pay" в базе
   **/
  static public function get_last_event_number() {
    $query = db_select('node', 'n')
      ->fields('n', array('nid', 'title'))
      ->condition('n.type', 'events_pay')
      ->orderBy('nid', 'DESC')
      ->execute()
      ->fetchAssoc();

    return (int)$query['title'];
  }

  /**
   * Получение количества денег в кошельке пользователя
   **/
  static public function get_user_balance($user) {
    $amount_wallet = field_get_items( 'user' , $user, 'payer_wallet' );
    if(empty($amount_wallet[0]['value'])) {
      $amount_wallet[0]['value'] = 0;
    }
    return $amount_wallet[0]['value'];
  }

  /**
   * Вычислить остаток на счету пользователя
   *
   * @param $act
   * @param $score
   * @param $amount
   *
   * @return int - balance
   */
  static public function calc_balance_user($act, $score, $amount) {
    if($act == 0) {
      $balance = $score - $amount;
    }
    else {
      $balance = $score + $amount;
    }
    return $balance;
  }

  /**
   * Установить статус материалу
   * @param $nid
   * @param $status
   */
  static public function set_event_pay_status($pay_id, $status, $nid = NULL) {
    if(!isset($nid)) {
      $nid = self::get_event_pay_nid($pay_id);
    }
    $node = (object)array(
      'nid' => $nid,
      'type' => 'events_pay',
    );

    $node->events_pay_status['und'][0]['value'] = $status;
    $field_info = field_info_field('events_pay_status');
    field_sql_storage_field_storage_write('node', $node, 'update', array($field_info['id']));
    cache_clear_all("field:node:{$node->nid}", 'cache_field');
  }

  /**
   * Запись вычисленного значения "$this->balance" в базу
   **/
  static function set_user_balance($user, $balance) {
    $user->payer_wallet['und'][0]['value'] = $balance;
    $field_info = field_info_field('payer_wallet');
    field_sql_storage_field_storage_write(
      'user',
      $user,
      'update',
      array($field_info['id'])
    );
    cache_clear_all("field:user:{$user->uid}", 'cache_field');
  }

  /**
   * Обработчик события с кошельком пользователя.
   * Тут нужно расширить функционал обработчиком ошибок и исключений,
   * Еще нужно рзабить метод на 4 метода: для рассчета остатка в кошельке
   * и создания счета в ожидании
   **/
  public function handler_event_pay() {
    if(!isset($this->pay_id)){
      $this->balance = self::calc_balance_user($this->act, $this->score, $this->amount);

      // если id платежа не передан, создаем событие
      if ($this->balance >= 0) {
        $this->create_event_pay_node();

        if($this->status == 1) {
          // Используется при прямом создании события через node add со статусом "Исполнено"
          self::set_user_balance($this->user, $this->balance);
        }
        drupal_set_message(t('Event successfully created'));
      }
      else {
        drupal_set_message(t('Insufficient funds!'), 'warning');
      }
    }
    else {
      if ($this->status == 1) {
        // если id платежа передан, нужно поменять ему статус и записать сумму на баланс пользователя
        $node = self::get_event_pay_node($this->pay_id);
        $this->act = $node->events_pay_act['und'][0]['value'];
        $this->amount = $node->events_pay_amount['und'][0]['value'];
        $this->user = self::get_user_object($node->uid);
        $this->score = self::get_user_balance($this->user);
        $this->balance = self::calc_balance_user($this->act, $this->score, $this->amount);

        self::set_event_pay_status('', $this->status, $node->nid);
        self::set_user_balance($this->user, $this->balance);

        drupal_set_message(t('Event "@pay_id" is "@status". @act = @amount. Current user balance @balance', array(
          '@pay_id' => $this->pay_id,
          '@status' => $this->status?'Completed':FALSE,
          '@act' => $this->act?'Coming':'Spending',
          '@amount' => $this->amount,
          '@balance' => $this->balance,
        )));
      }
      else {
        drupal_set_message(t('Оплата существует, но статус не корректный: "Ожидание"!'), 'warning');
      }
    }
  }

  /**
   * Создание события прихода или расхода средств
   **/
  private function create_event_pay_node() {
    $node = new stdClass();
    $node->type = 'events_pay';
    node_object_prepare($node);
    $node->title = self::get_last_event_number() + 1;
    $node->language = LANGUAGE_NONE;
    $node->uid = $this->user->uid;
    $node->events_pay_act[$node->language][0]['value'] = $this->act;
    $node->events_pay_status[$node->language][0]['value'] = $this->status;
    $node->events_pay_amount[$node->language][0]['value'] = $this->amount;
    $node->events_pay_comment[$node->language][0]['value'] = $this->comment;
    $node->status = 1;
    if ($node = node_submit($node)) {
      try {
        node_save($node);
        $this->pay_id = $node->title;
      }
      catch (Exception $e) {
        throw new Exception('Error saving node');
      }
    }
  }
}