<?php

class Payer {

  private $data = [];

  private $transaction = [];

  /**
   * Объект пользователя Drupal
   **/
  private $user;

  /**
   * Количество денег на счету пользователя поле "payer_wallet"
   **/
  private $balance;

  /**
   * Сколько денег останется после действий с кошельком:
   *   а) при расходе (0) - разность $wallet и $amount
   *   б) при приходе (1) - сумма $wallet и $amount
   * кошелек пользователя
   **/
  private $sum_balance;

  public function __construct($vars) {
    $this->data = $vars;
  }

  public function transaction() {
    /**
     * Если запрос на существующую транзакцию. Например: нужно получить данные по транзакции или поменять статус транзакции.
     */
    if (isset($this->data['tid'])) {
      // Если транзакция еще не была загружена в свойства объекта
      if(empty($this->transaction)) {
        $this->transaction = $this->load_transaction(); // Загружаем
      }

      // Смена статуса транзакции
      if (isset($this->data['status'])
        && $this->transaction['status'] != $this->data['status']
      ) {
        $this->save_transaction_status();
        $this->handler_user_balance();
      }
    }
    else {
      // Сохранение транзакции
      $this->save_transaction();
      $this->handler_user_balance();
    }
  }

  /**
   * Установить статус транзакции
   *
   * @param $status
   */
  public function set_transaction_status($status) {
    if(!empty($status)) {
      $this->data['status'] = $status;
    }
  }

  /**
   * Получить транзакцию из базы данных в виде массива
   */
  private function load_transaction() {
    return db_select('payer_transactions', 't')
      ->fields('t', [
        'tid',
        'uid',
        'status',
        'act',
        'amount',
        'created',
        'info',
        'method',
      ])
      ->condition('t.tid', $this->data['tid'])
      ->execute()
      ->fetchAssoc();
  }

  /**
   * Возвращает транзакцию из текущего объекта
   *
   * @return array
   */
  public function get_transaction() {
    return $this->transaction;
  }

  /**
   * Получить объект пользователя по uid
   **/
  private function get_user_object() {
    // Если uid не был передан загружаем текущего пользователя
    if (!isset($this->transaction['uid'])) {
      global $user;
      $this->transaction['uid'] = $user->uid;
    }
    return user_load($this->transaction['uid']);
  }


  /**
   * Сохранить статус транзакции
   */
  private function save_transaction_status() {
    db_update('payer_transactions')
      ->fields(['status' => $this->data['status']])
      ->condition('tid', $this->transaction['tid'])
      ->execute();
  }

  /**
   * Запись вычисленного значения "$this->sum_balance" в базу
   **/
  private function save_user_balance() {
    $this->user->payer_wallet['und'][0]['value'] = $this->sum_balance;
    $field_info = field_info_field('payer_wallet');

    field_sql_storage_field_storage_write(
      'user',
      $this->user,
      'update',
      [$field_info['id']]
    );

    cache_clear_all("field:user:{$this->user->uid}", 'cache_field');
  }

  private function save_transaction() {
    $this->transaction['uid'] = $this->data['uid'];
    $this->transaction['status'] = $this->data['status'];
    $this->transaction['act'] = $this->data['act'];
    $this->transaction['amount'] = $this->data['amount'];
    $this->transaction['method'] = $this->data['method'];
    $this->transaction['info'] = $this->data['info'];
    $this->transaction['created'] = $this->data['created'];

    db_insert('payer_transactions')
      ->fields($this->transaction)
      ->execute();
  }

  private function handler_user_balance() {
    // Обновляем баланс только, если статус транзакции "Завершено"
    if ($this->data['status'] == 1) {
      $this->user = $this->get_user_object();
      $this->balance = self::get_user_balance($this->user);

      $this->sum_balance = self::calc_sum_balance(
        $this->transaction['act'],
        $this->balance,
        $this->transaction['amount']
      );

      $this->save_user_balance();
    }
  }

  /**
   * Вычислить остаток на счету
   */
  static public function calc_sum_balance($act, $balance, $amount) {
    if ($act == 0) {
      $sum_balance = $balance - $amount;
    }
    else {
      $sum_balance = $balance + $amount;
    }

    return $sum_balance;
  }

  /**
   * Получение количества денег в кошельке пользователя
   **/
  static public function get_user_balance($user) {
    $amount_wallet = field_get_items(
      'user',
      $user,
      'payer_wallet'
    );

    if (empty($amount_wallet[0]['value'])) {
      $amount_wallet[0]['value'] = 0;
    }

    return $amount_wallet[0]['value'];
  }
}