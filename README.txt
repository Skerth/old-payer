Description:
-----------
This module allows you to create transactions for incoming and outgoing funds
on the site. Transactions can be created both manually and with the help of connected
payment gateways. All transactions are stored in the database table and can be
viewed at any time.

Requirements:
-----------
Drupal 7.x

Installation:
-----------
1. Copy the whole payer directory to the sites directory / all / modules

2. Log on to the system as an administrator. Enable the module on the
"Administration" -> "Modules". Module "Payer".

3. (Optional) Edit the module settings in the section
"Administration" -> "Configuration" -> "Payer" -> "Settings"

How to use:
-----------
To view all created transactions, go to the "Administration" -> "Configuration" -> "Payer"
section. Here you can filter transactions by parameters, and also go to
the viewing of a particular transaction.

When viewing the complete transaction information for a transaction with the status
"Pending" and the "Deposit" action, a link to the payment will be displayed
(if it is described in the function `payer_name_get_payment_link()`
of the connected payment method).

To create a transaction manually, go to the "Administration" -> "Configuration" -> "Payer"
section and click "Add transaction".

For normal account replenishment, go to the payment page (default: payment),
enter the amount and choose the payment method.

Developers:
-----------
New payment methods should be in the `methods` directory and implement the
following functions:

`payer_name_handler($uid, $amount)` - initializes the payment with the required
method, creates a transaction with the status "Pending" and redirects the user
to the payment gateway payment page.

`payer_robokassa_get_payment_link($transaction)` - generates a payment link so
that the user can return to the payment of the invoice if, for example, he
closes the tab with the payment page.

To implement payment methods, use the `PayerTransaction`
class (`includes/payer.transaction.inc`).
