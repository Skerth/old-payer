<div id="content" class="clearfix">
  <style>
    td.pay-method_status {
      font-size: 22px;
      cursor: default;
      -webkit-user-select: none;
      -moz-user-select: none;
      user-select: none;
    }
    td img {
      width: 100%;
      max-width: 125px;
      height: auto;
    }
  </style>
  <h2><?php print t('List of available payment methods'); ?></h2>
  <?php print theme_table($page_data); ?>
</div>