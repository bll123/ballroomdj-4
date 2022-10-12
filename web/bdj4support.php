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

if (isset($_FILES['upfile']['name']) &&
    $_FILES['upfile']['error'] == 0) {
  $tfn = $_FILES['upfile']['tmp_name'];
  $base = basename ($_POST['origfn']);
  $fn = $dir . '/' . $_POST['origfn'];
  $fdata = file_get_contents ($tfn);
  $fdata = base64_decode ($fdata);
  if (! file_exists (dirname ($fn))) {
    mkdir (dirname ($fn));
  }
  $gzfn = $fn . '.gz';
  if (file_exists ($gzfn)) { unlink ($gzfn); }
  file_put_contents ($gzfn, $fdata);
  if (file_exists ($tfn)) { unlink ($tfn); }
  if (file_exists ($fn)) { unlink ($fn); }
  system ("gzip -d $fn.gz");
  if (file_exists ($gzfn)) { unlink ($gzfn); }

  if ($base == 'support.txt') {
    $msg = file_get_contents ($fn);
    $headers = '';
    if (preg_match ("#Subject:\s*([^\r\n]*)[\r\n]#m", $msg, $matches)) {
      $subject = $matches[1];
    }
    // clean up any possible html tags
    preg_replace ('/</', $msg, '\&lt;');
    preg_replace ('/>/', $msg, '\&gt;');
    mail ('brad.lanam.di@gmail.com', "BDJ4: Support: $subject", $msg, $headers);
  }
  echo "OK";
} else {
  echo "NG: no upload file";
}

?>
