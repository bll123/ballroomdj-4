<?php

# v must always be present, get or post
if ( ! isset($_POST['v']) && ! isset ($_GET['v'])) {
  echo 'NG0';
  exit (0);
}
if (isset ($_GET['v']) && $_GET['v'] != 2) {
  echo 'NG1';
  exit (0);
}
if (isset ($_POST['v']) && $_POST['v'] != 2) {
  echo 'NG2';
  exit (0);
}

# tag must always be present, get or post
if ( ! isset($_POST['tag']) && ! isset ($_GET['tag'])) {
  echo 'NG3';
  exit (0);
}

# if the key is present, make sure it is correct.
if (isset($_POST['key']) && $_POST['key'] != '93457645') {
  echo 'NG4';
  exit (0);
}

# if the key is present, there must be marquee data
if (isset($_POST['key']) &&
   ! (isset($_POST['res']) || isset($_POST['mqdata']))
   ) {
  echo 'NG5';
  exit (0);
}

if (isset($_POST['key'])) {
  $tag = $_POST['tag'];
} else {
  $tag = $_GET['tag'];
}
$md = 'marquee_data';
$nfn = $md . '/' . $tag;
$kfn = $md . '/' . $tag . '.key';
$kres = '';

if (isset($_POST['key'])) {
  $secured = 'F';
  if ( file_exists($kfn) ) {
    $secured = 'T';
    if (! isset($_POST['userkey'])) {
      # the 'test2' tag is used by the test suite
      # if the 'test2' tag is present, pretend bdj is
      # not yet secured.
      if ($tag != 'test2') {
        echo 'NG6';
        exit (0);
      } else {
        # when using the 'test2' tag, re-use any existing file.
        $secured = 'R';
        $kres = file_get_contents ($kfn);
      }
    }
    if ($secured == 'T') {
      $key = file_get_contents ($kfn);
      # see if keys match
      if ($_POST['userkey'] != $key) {
        echo 'NG7';
        exit (0);
      }
    }
  }
  if ( $secured == 'F' ) {
    # first time, or never secured.
    $kres = mt_rand (100000,999999);
    if (file_put_contents ($kfn, $kres) === FALSE) {
      echo 'NG8';
      exit (0);
    }
  }
}

header ("Cache-Control: no-store");
header ("Expires: 0");

if (isset($_POST['key'])) {
  if (isset($_POST['res'])) {
    $res = $_POST['res'];
  }
  if (isset($_POST['mqdata'])) {
    $res = $_POST['mqdata'];
  }

  $tfn = $nfn . '.new';

  $rc = 'NG9';
  if (file_put_contents ($tfn, $res) !== FALSE) {
    if (rename ($tfn, $nfn)) {
      $rc = 'OK';
    }
  }
  if ( $kres != '' ) {
    # return the user key to the program.
    $rc = $kres;
  }
  echo $rc;
} else {
  if ( file_exists($nfn) ) {
    $res = file_get_contents ($nfn);
  } else {
    $res = '';
  }
  echo $res;
}

exit (0);

?>
