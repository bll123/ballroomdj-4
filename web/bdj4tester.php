<?php

$resp = "";
$resp .= "get: " . join ($_GET, ' ');
$resp .= "post: " . join ($_POST, ' ');
$resp .= "files: " . join ($_FILES, ' ');
if (! isset($_POST['key']) || $_POST['key'] != '8634556') {
  echo "NG: bad key: " . $resp;
  exit (0);
}

$ip = $_SERVER['REMOTE_ADDR'];

$dir = "tmptest";
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
    mkdir (dirname ($fn));
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
    system ("gzip -d $fn.gz");
    if (file_exists ($gzfn)) { unlink ($gzfn); }
  }
  echo "OK";
} else if (isset ($_POST['testdata'])) {
  echo "OK " . $_POST['testdata'];
} else {
  echo "NG: no directive: " . $resp;
}

?>
