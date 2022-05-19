<?php

function acomp ($a, $b) {
  $rc = - version_compare ($a, $b);
  return $rc;
}

$data = file_get_contents ('bdj4info.txt');
$darr = explode ("\n", $data);

$adata = array();

$html = <<<_HERE_
<html>
<head>
</head>
<body>
_HERE_;

$in = 0;
foreach ($darr as $line) {
  if (preg_match ("/^===END/", $line)) {
    $fver = $data['-version'] . '-' . $data['-releaselevel'] .
        '-' . $data['-builddate'];
    $adata[$fver]['-country'] = geoip_country_code_by_name ($data['-ip']);
    $adata[$fver]['-osdisp'] = $data['-osdisp'];
    $adata[$fver]['-date'] = $data['-date'];
    $adata[$fver]['-pythonvers'] = $data['-pythonvers'];
    $adata[$fver]['-locale'] = $data['-locale'];
    $adata[$fver]['-systemlocale'] = $data['-systemlocale'];
    $adata[$fver]['-oldversion'] = $data['-oldversion'];
    $adata[$fver]['-bdj3version'] = $data['-bdj3version'];
    $adata[$fver]['-new'] = $data['-new'];
    $adata[$fver]['-overwrite'] = $data['-overwrite'];
    $adata[$fver]['-update'] = $data['-update'];
    $adata[$fver]['-convert'] = $data['-convert'];
    $in = 0;
  }
  if ($in == 1) {
    $key = $line;
    $in = 2;
  } else if ($in == 2) {
    $data[$key] = $line;
    $in = 1;
  }
  if (preg_match ("/^===BEGIN/", $line)) {
    unset ($data);
    $data['-bdj3version'] = '';
    $data['-oldversion'] = '';
    $data['-locale'] = '';
    $data['-systemlocale'] = '';
    $in = 1;
  }
}

uksort ($adata, 'acomp');

$gdata = array ();

foreach ($adata as $vkey => $tdata) {
  if (empty ($gdata[$vkey])) {
    $gdata[$vkey]['-new'] = 0;
    $gdata[$vkey]['-overwrite'] = 0;
    $gdata[$vkey]['-update'] = 0;
    $gdata[$vkey]['-convert'] = 0;
    $gdata[$vkey]['-country'] = array ();
  }
  if (empty ($gdata[$vkey]['-country'][$tdata[-country]])) {
    $gdata[$vkey]['-country'][$tdata['-country']] = 0;
  }
  $gdata[$vkey]['-country'][$tdata['-country']] += 1;
  $gdata[$vkey]['-new'] += $tdata['-new'];
  $gdata[$vkey]['-overwrite'] += $tdata['-overwrite'];
  $gdata[$vkey]['-update'] += $tdata['-update'];
  $gdata[$vkey]['-convert'] += $tdata['-convert'];
}

$html .= <<<_HERE_
  <table>
    <tr>
      <th align="left">Version</th>
      <th align="left">Country</th>
      <th align="left">Count</th>
    </tr>
_HERE_;

foreach ($gdata as $vkey => $tdata) {
  foreach ($tdata['-country'] as $ckey => $count) {
    $html .= "    <tr>";
    $html .= "      <td align=\"left\">${vkey}</td>";
    $html .= "      <td align=\"left\">${ckey}</td>";
    $html .= "      <td align=\"right\">${count}</td>";
    $html .= "    </tr>";
  }
}

$html .= <<<_HERE_
  </table>
_HERE_;

$html .= <<<_HERE_
  <table>
    <tr>
      <th align="left">Version</th>
      <th align="left">New</th>
      <th align="left">Overwrite</th>
      <th align="left">Update</th>
      <th align="left">Convert</th>
    </tr>
_HERE_;

foreach ($gdata as $vkey => $tdata) {
  $html .= "    <tr>";
  $html .= "      <td align=\"left\">$vkey</td>";
  $html .= "      <td align=\"right\">${tdata['-new']}</td>";
  $html .= "      <td align=\"right\">${tdata['-overwrite']}</td>";
  $html .= "      <td align=\"right\">${tdata['-update']}</td>";
  $html .= "      <td align=\"right\">${tdata['-convert']}</td>";
  $html .= "    </tr>";
}

$html .= <<<_HERE_
  </table>
_HERE_;

$html .= <<<_HERE_
  <table>
    <tr>
      <th align="left">Version</th>
      <th align="left">Date</th>
      <th align="left">Country</th>
      <th align="left">OS</th>
      <th align="left">Python-Vers</th>
      <th align="left">Sys-Locale</th>
      <th align="left">Locale</th>
      <th align="left">New</th>
      <th align="left">Overwrite</th>
      <th align="left">Update</th>
      <th align="left">Convert</th>
      <th align="left">Old-Version</th>
      <th align="left">BDJ3-Version</th>
    </tr>
_HERE_;

$count = 0;
foreach ($adata as $vkey => $tdata) {
  $html .= "    <tr>";
  $html .= "      <td align=\"left\">$vkey</td>";
  $html .= "      <td align=\"left\">${tdata['-date']}</td>";
  $html .= "      <td align=\"left\">${tdata['-country']}</td>";
  $html .= "      <td align=\"left\">${tdata['-osdisp']}</td>";
  $html .= "      <td align=\"right\">${tdata['-pythonvers']}</td>";
  $html .= "      <td align=\"right\">${tdata['-systemlocale']}</td>";
  $html .= "      <td align=\"right\">${tdata['-locale']}</td>";
  $html .= "      <td align=\"right\">${tdata['-new']}</td>";
  $html .= "      <td align=\"right\">${tdata['-overwrite']}</td>";
  $html .= "      <td align=\"right\">${tdata['-update']}</td>";
  $html .= "      <td align=\"right\">${tdata['-convert']}</td>";
  $html .= "      <td align=\"left\">${tdata['-oldversion']}</td>";
  $html .= "      <td align=\"left\">${tdata['-bdj3version']}</td>";
  $html .= "    </tr>";
  ++$count;
  if ($count > 30) {
    break;
  }
}

$html .= <<<_HERE_
  </table>
_HERE_;

$html .= <<<_HERE_
</body>
</html>
_HERE_;
print $html;

?>
