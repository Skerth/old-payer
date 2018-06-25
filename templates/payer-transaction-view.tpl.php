<style>
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
<table>
  <?php
  $i = 0;
  foreach($page_data as $data):
    if($i % 2) { $class = ' even'; }
    else { $class = ' odd'; }
  ?>
    <tr class="row <?php print $class; ?>">
      <th class="label" width="100">
        <?php print $data['label']; ?>
      </th>
      <td class="item-data<?php isset($data['class']) ? print $data['class'] : NULL ?>">
        <?php print $data['value']; ?>
      </td>
    </tr>
  <?php
    $i++;
  endforeach;
  ?>
</table>
