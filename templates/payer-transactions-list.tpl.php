<div id="content" class="clearfix">
  <style>
    table tr:hover td {
      background: #e0e1db !important;
    }
    .transaction-amount:before {
      font-size: 20px;
      vertical-align: top;
    }
    .transaction-amount-0:before {
      content: '⬆ ';
      color: #ed541d;
    }
    .transaction-amount-1:before {
      content: '⬇ ';
      color: #008000;
    }
  </style>
  <h2><?php print t('List of transactions'); ?></h2>
  <?php print $page_data; ?>
</div>