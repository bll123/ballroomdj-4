<?php

# tag must always be present, get or post
if ( ! isset($_POST['mobmqtag']) && ! isset ($_GET['mobmqtag'])) {
  echo 'NG1';
  exit (0);
}

# if the key is present, make sure it is correct.
# the key is only present for a post action
if (isset($_POST['key']) && $_POST['key'] != '73459734') {
  echo 'NG2';
  exit (0);
}

# if the mobmq-key is present, there must be marquee data
if (isset($_POST['mobmqkey']) &&
   ! (isset($_POST['res']) || isset($_POST['mqdata']))
   ) {
  echo 'NG3';
  exit (0);
}

if (isset($_POST['mobmqkey'])) {
  $tag = $_POST['mobmqtag'];
} else {
  $tag = $_GET['mobmqtag'];
}
$md = 'bdj4mqdata';
$nfn = $md . '/' . $tag;
$kfn = $md . '/' . $tag . '.key';
$kres = '';
$secured = 'F';

# if the mobmq-key is present, check and see if there is a .key file
# if so, the mobmq-key must match the contents of the .key file
if (isset($_POST['mobmqkey'])) {
  if ( file_exists($kfn) ) {
    $key = file_get_contents ($kfn);
    # see if keys match
    if ($_POST['mobmqkey'] != $key) {
      echo 'NG4';
      exit (0);
    }
    $secured = 'T';
  } else {
    /* if the .key file is not present, write the key out to the file */
    if (file_put_contents ($kfn, $key) === false) {
      echo 'NG5';
      exit (0);
    }
    $secured = 'T';
  }
}

header ("Cache-Control: no-store");
header ("Expires: 0");

if (isset($_POST['mobmqkey'])) {
  if ($secured == 'T') {
    if (isset($_POST['res'])) {
      $res = $_POST['res'];
    }
    if (isset($_POST['mqdata'])) {
      $res = $_POST['mqdata'];
    }

    $tfn = $nfn . '.new';

    $rc = 'NG6';
    if (file_put_contents ($tfn, $res) !== FALSE) {
      if (rename ($tfn, $nfn)) {
        $rc = 'OK';
      }
    }
  } else {
    $rc = 'NG7';
  }
  echo $rc;
} else {
  /* normal user fetch of marquee */
  if ( file_exists($nfn) ) {
    $res = file_get_contents ($nfn);
  } else {
    $res = '';
  }
  echo $res;
}

exit (0);

?>
