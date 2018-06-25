<?php

/**
 * @file
 * The main class of the Payer for working with transactions
 */

class PayerTransaction {

  /**
   * Data received by the object PayerTransaction.
   * Possible array keys:
   *   - 'tid': transaction TID. Transmitted if you want to work with an
   *      existing transaction;
   *   - 'uid': user UID. Example: $user->uid;
   *   - 'status': transaction status. Can be "0" (Waiting) or "1" (Completed);
   *   - 'act': transaction action. Can be "0" (Spending) or "1" (Coming);
   *   - 'amount': the amount of replenishment or cancellation from the user's
   *      wallet. Should be a floating number with a decimal separator
   *      in the form of a dot;
   *   - 'method': selected payment method. Plain text string.
   *      Example: Manual, Robokassa, PayPal;
   *   - 'info': array of values required for a specific payment method;
   *   - 'created': date of transaction creation. Should be in Unixtime.
   */
  private $data = [];

  /**
   * All properties of the current object. Contains the same list
   * as @var array $data.
   *
   * @see get_transaction().
   *
   */
  private $transaction = [];

  /**
   * User object Drupal.
   **/
  private $user;

  /**
   * The amount of money on the user's account. User field "payer_wallet".
   **/
  private $balance;

  /**
   * How much money will remain after the action with the wallet:
   *   a) when wasting money: ['act'] = 0 - difference $wallet and $amount;
   *   b) when replenishing money: ['act'] = 1 - sum $wallet and $amount.
   **/
  private $sum_balance;

  public function __construct($vars) {
    $this->data = $vars;
    if(!isset($this->data['info'])) {
      $this->data['info'] = array();
    }
  }

  /**
   * Main transaction handler.
   *
   * @see load_transaction()
   * @see save_transaction()
   * @see handler_user_balance()
   * @see save_transaction_status()
   */
  public function transaction() {
    /**
     * If the request for an existing transaction. Example: you need to get
     * transaction data or change the status of a transaction.
     */
    if (isset($this->data['tid'])) {
      // If the transaction has not yet been loaded into the properties
      // of the object
      if(empty($this->transaction)) {
        $this->transaction = $this->load_transaction(); // download.
      }

      // Change of transaction status.
      if (isset($this->data['status'])
        && $this->transaction['status'] != $this->data['status']
      ) {
        $this->save_transaction_status();
        $this->handler_user_balance();
      }
    }
    else {
      // Saving a transaction.
      $this->save_transaction();
      $this->handler_user_balance();
    }
  }

  /**
   * Set transaction status.
   *
   * @param $status - transaction status.
   *        Can be "0" (Waiting) or "1" (Completed).
   */
  public function set_transaction_status($status) {
    if(!empty($status)) {
      $this->data['status'] = $status;
    }
  }

  /**
   * Get a transaction from the database as an array.
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
   * Returns transaction properties from the current object as an array.
   *
   * @return array $this->transaction.
   */
  public function get_transaction() {
    $this->transaction['info'] = unserialize($this->transaction['info']);

    return $this->transaction;
  }

  /**
   * Get a user object by UID.
   **/
  private function get_user_object() {
    // If uid has not been transferred, load the current user.
    if (!isset($this->transaction['uid'])) {
      global $user;
      $this->transaction['uid'] = $user->uid;
    }
    return user_load($this->transaction['uid']);
  }

  /**
   * Save transaction status.
   */
  private function save_transaction_status() {
    db_update('payer_transactions')
      ->fields(['status' => $this->data['status']])
      ->condition('tid', $this->transaction['tid'])
      ->execute();
  }

  /**
   * Record the calculated value of "$this->sum_balance" in the database.
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

  /**
   * Saving a new transaction to the database.
   */
  private function save_transaction() {
    $this->transaction['uid'] = $this->data['uid'];
    $this->transaction['status'] = $this->data['status'];
    $this->transaction['act'] = $this->data['act'];
    $this->transaction['amount'] = $this->data['amount'];
    $this->transaction['method'] = $this->data['method'];
    $this->transaction['info'] = serialize($this->data['info']);
    $this->transaction['created'] = $this->data['created'];

    $tid = db_insert('payer_transactions')
      ->fields($this->transaction)
      ->execute();

    $this->transaction['tid'] = $tid;
  }

  /**
   * Processes user balance. Performed if the status of transaction 1 (Completed).
   *
   * @see get_user_object()
   * @see get_user_balance()
   * @see calc_sum_balance()
   * @see save_user_balance()
   */
  private function handler_user_balance() {
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
   * Calculate account balance
   * @param $act - transaction action. Can be "0" (Spending) or "1" (Coming);
   * @param $balance - current user balance;
   * @param $amount - amount of replenishment or write-offs.
   *
   * @return float $sum_balance - the value obtained.
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
   * Getting the amount of money in the user's wallet.
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
