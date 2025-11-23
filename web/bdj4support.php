<?php

if (! isset($_POST['key']) || $_POST['key'] != '9034545') {
  echo "NG: bad key";
  exit (0);
}
if (! isset($_POST['ident'])) {
  echo "NG: no ident";
  exit (0);
}

$ip = $_SERVER['REMOTE_ADDR'];
$ident = $_POST['ident'];

$dir = "support/$ident";
if (! file_exists($dir)) {
  mkdir ($dir, 0755, true);
}

$gzipped = false;
if (isset($_FILES['upfile']['name']) &&
    $_FILES['upfile']['error'] == 0) {
  $tfn = $_FILES['upfile']['tmp_name'];
  $base = basename ($_POST['origfn']);
  $fn = $dir . '/' . $_POST['origfn'];
  $fdata = file_get_contents ($tfn);
  if (preg_match ("/\.b64/", $_FILES['upfile']['name'])) {
    $fdata = base64_decode ($fdata);
  }
  if (! file_exists (dirname ($fn))) {
    mkdir (dirname ($fn), 0755, true);
  }
  if (preg_match ("/\.gz/", $_FILES['upfile']['name'])) {
    $gzipped = true;
  }

  if ($gzipped) {
    $gzfn = $fn . '.gz';
    if (file_exists ($gzfn)) { unlink ($gzfn); }
    file_put_contents ($gzfn, $fdata);
  } else {
    file_put_contents ($fn, $fdata);
  }
  if (file_exists ($tfn)) { unlink ($tfn); }
  if ($gzipped) {
    if (file_exists ($fn)) { unlink ($fn); }
    if (file_exists ($gzfn)) {
      system ("gzip -d $fn.gz");
    }
    if (file_exists ($gzfn)) {
      unlink ($gzfn);
    }
  }
  if ($base == "support.txt") {
    $msg = '';
    $msg .= $ident . "\n";
    $msg .= "\n";
    $msg .= file_get_contents ($fn);
    $headers = '';
    mail ('brad.lanam.di@gmail.com', "BDJ4: Support: $ident", $msg, $headers);
  }
  echo "OK";
} else {
  echo "NG: no upload file";
}

?>
