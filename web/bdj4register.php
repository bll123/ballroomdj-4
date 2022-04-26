<?php

if (! isset($_POST['key']) || $_POST['key'] != '9873453') {
  echo "NG: bad key";
  exit (0);
}
if (! isset($_POST['version'])) {
  echo "NG: no versoin";
  exit (0);
}

$ip = $_SERVER['REMOTE_ADDR'];
$ofile = 'bdj4info.txt';
$date = date ('Y-m-d H:m');

$str = '';
$str .= "===BEGIN\n";
$str .= "ip\n" . $ip . "\n";
$str .= "date\n" . $date . "\n";
foreach ($_POST as $key => $value) {
  if ($key == 'key') {
    continue;
  }
  $str .= $key . "\n";
  $str .= $value . "\n";
}
$str .= "===END\n";
file_put_contents ($ofile, $str, FILE_APPEND);

?>
