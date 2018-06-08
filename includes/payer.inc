<?php

class Payer {
  /**
   * Объект пользователя Drupal
   **/
  private $user;

  /**
   * Количество денег на счету пользователя поле "payer_wallet"
   **/
  private $wallet = NULL;

  /**
   * Действие с кошельком пользователя:
   *  0 - Расход (списание)
   *  1 - Приход (пополнение)
   **/
  private $act = NULL;

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
  private $calc_wallet = NULL;

  /**
   * Комментарий к платежу
   **/
  private $comment = NULL;

  public function __construct($vars) {
    // Если uid не был передан загружаем текущего пользователя
    if (!isset($vars['uid'])){
      global $user;
      $uid = $user->uid;
    }
    else {
      $uid = $vars['uid'];
    }
    $this->user = user_load($uid);
    $this->wallet = $this->get_user_amount_wallet();

    if (isset($vars['act'])){
      $this->act = $vars['act'];
    }
    if (isset($vars['amount'])){
      $this->amount = $vars['amount'];
    }
    if (isset($vars['comment'])){
      $this->comment = $vars['comment'];
    }
  }

  /**
   * Обработчик события с кошельком пользователя.
   * Тут нужно расширить функционал обработчиком ошибок и исключений
   **/
  public function handler_event_pay() {
    if ($this->act == 0) {
      $this->calc_wallet = $this->wallet - $this->amount;
    }
    else {
      $this->calc_wallet = $this->wallet + $this->amount;
    }

    if ($this->calc_wallet >= 0) {
      $this->create_event_pay_node();
      $this->set_user_calc_wallet();
      drupal_set_message(t('Event successfully created'));
    }
    else {
      drupal_set_message(t('Insufficient funds!'), 'warning');
    }
  }

  /**
   * Создание события прихода или расхода средств
   **/
  private function create_event_pay_node() {
    $node = new stdClass();
    $node->type = 'events_pay';
    node_object_prepare($node);
    $node->title = $this->get_amount_events_pay() + 1;
    $node->language = LANGUAGE_NONE;
    $node->uid = $this->user->uid;
    $node->events_pay_act[$node->language][0]['value'] = $this->act;
    $node->events_pay_amount[$node->language][0]['value'] = $this->amount;
    $node->events_pay_comment[$node->language][0]['value'] = $this->comment;
    $node->status = 1;
    if ($node = node_submit($node)) {
      try {
        node_save($node);
      }
      catch (Exception $e) {
        throw new Exception('Error saving node');
      }
    }
  }

  /**
   * Запись вычисленного значения "$this->calc_wallet" в базу
   **/
  private function set_user_calc_wallet() {
    $this->user->payer_wallet['und'][0]['value'] = $this->calc_wallet;
    $field_info = field_info_field('payer_wallet');
    field_sql_storage_field_storage_write(
      'user',
      $this->user,
      'update',
      array($field_info['id'])
    );
    cache_clear_all("field:user:{$this->user->uid}", 'cache_field');
  }

  /**
   * Получение количества материалов типа "events_pay" в базе
   **/
  private function get_amount_events_pay() {
    return db_select('node', 'n')
      ->condition('n.type', 'events_pay')
      ->countQuery()
      ->execute()
      ->fetchField();
  }

  /**
   * Получение количества денег в кошельке пользователя
   **/
  private function get_user_amount_wallet() {
    $amount_wallet = field_get_items( 'user' , $this->user, 'payer_wallet' );
    return $amount_wallet[0]['value'];
  }
}