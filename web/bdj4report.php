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

$gidx = 0;
$in = 0;
foreach ($darr as $line) {
  if (preg_match ("/^===END/", $line)) {
    $fver = $idata['-version'] . '-' . $idata['-releaselevel'] .
        '-' . $idata['-builddate'];
    $idata['-fullversion'] = $fver;

    $idata['-country'] = 'unknown';
    if (1) {
      $turl = 'https://ipinfo.io/' .
          $idata['-ip'] . '?token=3e323f77a7a9fd';
      $curl = curl_init ($turl);
      curl_setopt ($curl, CURLOPT_RETURNTRANSFER, true);
      $resp = curl_exec ($curl);
      curl_close ($curl);
      $ipdata = json_decode ($resp, true);

      if (! empty ($ipdata)) {
        $c = $ipdata ['country'];
        $idata['-country'] = $c;
      }
    }

    if (isset ($idata['-oldversion'])) {
      preg_replace ('/ /', $idata['-oldversion'], '-');
    }
    if (isset ($idata['-overwrite'])) {
      $idata['-reinstall'] = $idata['-overwrite'];
    }
    if (isset ($idata['-reinstall']) && isset ($idata['-new'])) {
      $idata['-reinstall'] = 0;
    }
    $in = 0;
    $adata[$gidx] = $idata;
    $gidx++;
  }
  if ($in == 1) {
    $key = $line;
    $in = 2;
  } else if ($in == 2) {
    $idata[$key] = $line;
    $in = 1;
  }
  if (preg_match ("/^===BEGIN/", $line)) {
    unset ($idata);
    $idata['-bdj3version'] = '';
    $idata['-oldversion'] = '';
    $idata['-locale'] = '';
    $idata['-systemlocale'] = '';
    $in = 1;
  }
}

foreach ($adata as $gidx => $tdata) {
  $fver = $tdata['-fullversion'];
  foreach ($tdata as $key => $val) {
    if ($key == '-country') {
      if (! isset ($vdata[$fver][$key][$val])) {
        $vdata[$fver][$key][$val] = 0;
      }
      $vdata[$fver][$key][$val] += 1;
    } else if (in_array ($key, array ('-new', '-reinstall', '-update', '-convert'), true)) {
      if (! isset ($vdata[$fver][$key])) {
        $vdata[$fver][$key] = 0;
      }
      $vdata[$fver][$key] += $val;
    } else {
      $vdata[$fver][$key] = $val;
    }
  }
}

uksort ($vdata, 'acomp');

$html .= <<<_HERE_
  <table>
    <tr>
      <th align="left">Version</th>
      <th align="left">Country</th>
      <th align="left">Count</th>
    </tr>
_HERE_;

foreach ($vdata as $fkey => $tdata) {
  foreach ($tdata['-country'] as $ckey => $count) {
    $html .= "    <tr>";
    $html .= "      <td align=\"left\">${fkey}</td>";
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
      <th align="left">Re-Install</th>
      <th align="left">Update</th>
      <th align="left">Convert</th>
    </tr>
_HERE_;

foreach ($vdata as $fkey => $tdata) {
  $html .= "    <tr>";
  $html .= "      <td align=\"left\">$fkey</td>";
  $html .= "      <td align=\"right\">${tdata['-new']}</td>";
  $html .= "      <td align=\"right\">${tdata['-reinstall']}</td>";
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
      <th align="left">Re-Install</th>
      <th align="left">Update</th>
      <th align="left">Convert</th>
      <th align="left">Old-Version</th>
      <th align="left">BDJ3-Version</th>
    </tr>
_HERE_;

$count = 0;
foreach (array_reverse ($adata) as $gidx => $tdata) {
  $html .= "    <tr>";
  $html .= "      <td align=\"left\">${tdata['-fullversion']}</td>";
  $html .= "      <td align=\"left\">${tdata['-date']}</td>";
  $html .= "      <td align=\"left\">${tdata['-country']}</td>";
  $html .= "      <td align=\"left\">${tdata['-osdisp']}</td>";
  $html .= "      <td align=\"right\">${tdata['-pythonvers']}</td>";
  $html .= "      <td align=\"right\">${tdata['-systemlocale']}</td>";
  $html .= "      <td align=\"right\">${tdata['-locale']}</td>";
  $html .= "      <td align=\"right\">${tdata['-new']}</td>";
  $html .= "      <td align=\"right\">${tdata['-reinstall']}</td>";
  $html .= "      <td align=\"right\">${tdata['-update']}</td>";
  $html .= "      <td align=\"right\">${tdata['-convert']}</td>";
  $html .= "      <td align=\"left\">${tdata['-oldversion']}</td>";
  $html .= "      <td align=\"left\">${tdata['-bdj3version']}</td>";
  $html .= "    </tr>";
  $count++;
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
