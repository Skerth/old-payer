<table>
  <?php
  $i = 0;
  foreach($transaction as $key => $data):
    if($i % 2) {
      $class = ' even';
    }
    else {
      $class = ' odd';
    }
    ?>
    <tr class="row <?php print $class; ?>">
      <th class="label" width="100">
        <?php print $key; ?>
      </th>
      <td class="item-data">
        <?php
        switch ($key){
          case 'uid':
            $user = user_load($data);
            print $data. ' (' . l($user->name, 'user/'.$data) . ')';
            break;
          case 'created':
            print format_date($data, 'short');
            break;
          case 'act':
            print $data?'Coming':'Spending';
            break;
          case 'status':
            print $data?'Completed':'Waiting';
            break;
          default:
            print $data;
            break;
        }
        ?>
      </td>
    </tr>
    <?php
    $i++;
  endforeach;
  ?>
</table>