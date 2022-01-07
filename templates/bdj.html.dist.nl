<!-- Nederlands HTML 5 -->
<!DOCTYPE html>
<!--
http://danielstern.ca/range.css/#/
-->
<!-- VERSION 2010-10-31H -->
<html>
<head>
  <title>BallroomDJ</title>
  <meta http-equiv="Content-Type" content="text/html;charset=utf-8" >
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="Description" content="BallroomDJ Web Interface">
  <style type="text/css">
    body {
      background-color: white;
      margin: 0px;
      padding: 0px;
      font-size: 100%;
    }
    table {
      font-size: 100%;
    }
    p {
      margin: 0;
      padding: 0;
      font-size: 100%;
    }
    img {
      border: 0;
    }
    .imgmiddle {
      align: middle;
    }
    button {
      margin-left: 5pt;
      border-radius: 0;
      -webkit-border-radius: 0;
      -webkit-appearance: none;
      background-color: #d5b5ff;
    }
    .mutebutton {
      margin-left: 5pt;
      margin-right: 0;
      margin-top: 0;
      margin-bottom: 0;
      padding-top: 5px;
      padding-left: 4px;
      padding-right: 4px;
      padding-bottom: 0px;
    }
    input.button:focus {
      outline-width: 0;
      color: black;
    }
    hr {
      background-color: #d5b5ff;
    }
    select {
      background-color: #d5b5ff;
      appearance: none;
      font-size: 110%;
    }
    input {
      outline: none;
      padding: 0.5em;
      font-size: 100%;
      border-radius: 0;
      background-color: #ffffff;
      -webkit-border-radius: 0;
      -webkit-appearance: none;
    }
    .divinline {
      display: inline;
      clear: none;
    }
    .divnone {
      display: none;
      clear: none;
    }
    .mpadr {
      margin-right: 10pt;
    }
    .wb {
      width: 100%;
      background-color: #d5b5ff;
    }
    .divcenter {
      display: block;
      width: 98%;
      max-width: 12cm;
      margin: 0 auto;
    }
    .center {
      text-align: center;
    }
    .pbold {
      font-weight: bold;
    }
    .heading {
      font-size: 140%;
    }
    .table {
      table-layout: auto;
      width: 100%;
      border: 0;
    }
    td {
      padding-top: 2mm;
      padding-right: 2mm;
    }
    .txtcenter {
      text-align: center;
    }
    .txtleft {
      text-align: left;
    }
    .vcenter {
      vertical-align: middle;
    }
    .tdpl {
      padding-left: 50px;
    }
    .tdwide {
      display: table-cell;
      width: 100%;
    }
    .tdnopad {
      padding: 0;
    }
    .tdnarrow {
      display: table-cell;
      width: 8%;
    }
    input[type=range] {
      -webkit-appearance: none;
      width: 100%;
      margin: 1.3px 0;
    }
    input[type=range]:focus {
      outline: none;
    }
    input[type=range]::-webkit-slider-runnable-track {
      width: 100%;
      height: 8.4px;
      cursor: pointer;
      box-shadow: 0px 0px 1px #000000, 0px 0px 0px #0d0d0d;
      background: #9056a5;
      border-radius: 4.5px;
      border: 0px solid #010101;
    }
    input[type=range]::-webkit-slider-thumb {
      box-shadow: 1px 1px 2px #9056a5, 0px 0px 1px #9b66af;
      border: 0.3px solid rgba(144, 86, 165, 0.84);
      height: 11px;
      width: 50px;
      border-radius: 47px;
      background: rgba(255, 0, 117, 0.53);
      cursor: pointer;
      -webkit-appearance: none;
      margin-top: -1.3px;
    }
    input[type=range]:focus::-webkit-slider-runnable-track {
      background: #9b66af;
    }
    input[type=range]::-moz-range-track {
      width: 100%;
      height: 8.4px;
      cursor: pointer;
      box-shadow: 0px 0px 1px #000000, 0px 0px 0px #0d0d0d;
      background: #9056a5;
      border-radius: 4.5px;
      border: 0px solid #010101;
    }
    input[type=range]::-moz-range-thumb {
      box-shadow: 1px 1px 2px #9056a5, 0px 0px 1px #9b66af;
      border: 0.3px solid rgba(144, 86, 165, 0.84);
      height: 11px;
      width: 50px;
      border-radius: 47px;
      background: rgba(255, 0, 117, 0.53);
      cursor: pointer;
    }
    input[type=range]::-ms-track {
      width: 100%;
      height: 8.4px;
      cursor: pointer;
      background: transparent;
      border-color: transparent;
      color: transparent;
    }
    input[type=range]::-ms-fill-lower {
      background: #814d94;
      border: 0px solid #010101;
      border-radius: 9px;
      box-shadow: 0px 0px 1px #000000, 0px 0px 0px #0d0d0d;
    }
    input[type=range]::-ms-fill-upper {
      background: #9056a5;
      border: 0px solid #010101;
      border-radius: 9px;
      box-shadow: 0px 0px 1px #000000, 0px 0px 0px #0d0d0d;
    }
    input[type=range]::-ms-thumb {
      box-shadow: 1px 1px 2px #9056a5, 0px 0px 1px #9b66af;
      border: 0.3px solid rgba(144, 86, 165, 0.84);
      height: 11px;
      width: 50px;
      border-radius: 47px;
      background: rgba(255, 0, 117, 0.53);
      cursor: pointer;
      height: 8.4px;
    }
    input[type=range]:focus::-ms-fill-lower {
      background: #9056a5;
    }
    input[type=range]:focus::-ms-fill-upper {
      background: #9b66af;
    }
    input[type=range] {
      -webkit-appearance: none;
      width: 100%;
      margin: 1.3px 0;
    }
    input[type=range]:focus {
      outline: none;
    }
    input[type=range]::-webkit-slider-runnable-track {
      width: 100%;
      height: 8.4px;
      cursor: pointer;
      box-shadow: 0px 0px 1px #000000, 0px 0px 0px #0d0d0d;
      background: #9056a5;
      border-radius: 4.5px;
      border: 0px solid #010101;
    }
    input[type=range]::-webkit-slider-thumb {
      box-shadow: 1px 1px 2px #9056a5, 0px 0px 1px #9b66af;
      border: 0.3px solid rgba(144, 86, 165, 0.84);
      height: 11px;
      width: 50px;
      border-radius: 47px;
      background: rgba(255, 0, 117, 0.53);
      cursor: pointer;
      -webkit-appearance: none;
      margin-top: -1.3px;
    }
    input[type=range]:focus::-webkit-slider-runnable-track {
      background: #9b66af;
    }
    input[type=range]::-moz-range-track {
      width: 100%;
      height: 8.4px;
      cursor: pointer;
      box-shadow: 0px 0px 1px #000000, 0px 0px 0px #0d0d0d;
      background: #9056a5;
      border-radius: 4.5px;
      border: 0px solid #010101;
    }
    input[type=range]::-moz-range-thumb {
      box-shadow: 1px 1px 2px #9056a5, 0px 0px 1px #9b66af;
      border: 0.3px solid rgba(144, 86, 165, 0.84);
      height: 11px;
      width: 50px;
      border-radius: 47px;
      background: rgba(255, 0, 117, 0.53);
      cursor: pointer;
    }
    input[type=range]::-ms-track {
      width: 100%;
      height: 8.4px;
      cursor: pointer;
      background: transparent;
      border-color: transparent;
      color: transparent;
    }
    input[type=range]::-ms-fill-lower {
      background: #814d94;
      border: 0px solid #010101;
      border-radius: 9px;
      box-shadow: 0px 0px 1px #000000, 0px 0px 0px #0d0d0d;
    }
    input[type=range]::-ms-fill-upper {
      background: #9056a5;
      border: 0px solid #010101;
      border-radius: 9px;
      box-shadow: 0px 0px 1px #000000, 0px 0px 0px #0d0d0d;
    }
    input[type=range]::-ms-thumb {
      box-shadow: 1px 1px 2px #9056a5, 0px 0px 1px #9b66af;
      border: 0.3px solid rgba(144, 86, 165, 0.84);
      height: 11px;
      width: 50px;
      border-radius: 47px;
      background: rgba(255, 0, 117, 0.53);
      cursor: pointer;
      height: 8.4px;
    }
    input[type=range]:focus::-ms-fill-lower {
      background: #9056a5;
    }
    input[type=range]:focus::-ms-fill-upper {
      background: #9b66af;
    }
  </style>
</head>
<body>
  <div id="top" class="divcenter">
    <table class="table">
      <tr>
        <td class="txtleft">
          <img class="vcenter" src="ballroomdj.svg"
            width="200" alt="BallroomDJ">
        </td>
        <td class="vcenter txtcenter tdnopad">
            <img id="nextpagei" onclick="javascript:bdj.nextPage();"
              src="mrc/light/nextpage.svg" alt="next page">
        </td>
      </tr>
    </table>
  </div>
  <div id="controlspage1" class="divcenter">
    <table class="table">
      <tr>
        <td class="tdnarrow">
          <p>Volume</p>
        </td>
        <td class="txtcenter" colspan="3">
          <input class="vcenter" id="volumeb" type="range"
            onmousedown="javascript:bdj.chgStart();"
            onmouseup="javascript:bdj.chgEnd();"
            oninput="javascript:bdj.chgVolume();"
            onchange="javascript:bdj.chgVolume();"></p>
        </td>
        <td class="vcenter tdnopad">
          <button class="mutebutton" id="muteb" type="button"
            onclick="javascript:bdj.chgMute();"><img
            id="mutei" src="mrc/light/mute.svg" alt="mute"></button>
        </td>
        <td colspan="2">
          <p class="vcenter txtleft" id="vold">70%</p>
        </td>
      </tr>
      <tr>
        <td colspan="4">
          <input class="wb" type="submit"
            onclick="javascript:bdj.sendCmd('play');"
            name="playpause" value="Afspelen/Pauseren">
        </td>
        <td class="vcenter">
          <img class="vcenter" id="playstatusi"
            src="mrc/light/stop.svg" alt="stop">
        </td>
        <td class="vcenter">
          <div class="divnone" id="pausestatusd">
            <img class="vcenter" id="pausestatusi"
              src="mrc/light/pause.svg" alt="pause">
          </div>
          <div class="divinline" id="pauseblankd">
            &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
          </div>
        </td>
      </tr>
      <tr>
        <td colspan="4">
          <input class="wb" type="submit" id="fadeb"
            onclick="javascript:bdj.sendCmd('fade');"
            name="fade" value="Fade">
        </td>
        <td colspan="2"></td>
      </tr>
      <tr>
        <td colspan="4">
          <input class="wb" type="submit"
            onclick="javascript:bdj.sendCmd('repeat');"
            name="repeat" value="Herhalen">
        </td>
        <td class="vcenter" colspan="2">
          <div class="divnone" id="repeatstatusd">
            <img class="vcenter" id="repeatstatusi"
              src="mrc/light/repeat.svg" alt="repeat">
          </div>
          <div class="divinline" id="repeatblankd">
            &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
          </div>
        </td>
      </tr>
      <tr>
        <td colspan="4">
          <input class="wb" type="submit"
            onclick="javascript:bdj.sendCmd('pauseatend');"
            name="pauseatend" value="Pauzeer aan einde">
        </td>
        <td class="vcenter" colspan="2">
          <img class="vcenter" id="pauseatendi"
            src="led_off.svg" alt="pause-at-end">
        </td>
      </tr>
      <tr>
        <td colspan="4">
          <input class="wb" type="submit"
            onclick="javascript:bdj.sendCmd('nextsong');"
            name="nextsong" value="Volgende nummer">
        </td>
      </tr>
      <tr>
        <td class="tdnarrow">
          <p>Speed</p>
        </td>
        <td class="txtcenter" colspan="4">
          <input class="vcenter mpadr" id="speedb" type="range"
            min="70" max="130" value="100"
            onmousedown="javascript:bdj.chgStart();"
            onmouseup="javascript:bdj.chgEnd();"
            oninput="javascript:bdj.chgSpeed();"
            onchange="javascript:bdj.chgSpeed();"></p>
        </td>
        <td colspan="1">
          <p class="vcenter" id="speedd">100%</p>
        </td>
      </tr>
      <tr>
        <td colspan="6">
          <p class="pbold" id="dance"></p>
        </td>
      </tr>
      <tr>
        <td colspan="6">
          <p class="pbold" id="artist"></p>
        </td>
      </tr>
      <tr>
        <td colspan="6">
          <p class="pbold" id="title"></p>
        </td>
      </tr>
    </table>
  </div>
  <div id="controlspage2" class="divnone">
    <p class="txtcenter pbold heading">Quick Play</p>
    <p class="">&nbsp;</p>
    <select id="dancelist" name="dancelist"></select>
    <input class="wb" type="submit"
      onclick="javascript:bdj.sendQuickplayCmd('qpqueue');"
      name="queue" value="Wachtrij">
    <input class="wb" type="submit"
      onclick="javascript:bdj.sendQuickplayCmd('qpplay5');"
      name="play5" value="Play 5">
    <input class="wb" type="submit"
      onclick="javascript:bdj.sendQuickplayCmd('qpplaymany');"
      name="playmany" value="Play Continuously">
    <input class="wb" type="submit"
      onclick="javascript:bdj.sendCmd('qpclear');"
      name="clear" value="Clear Player">
    <input class="wb" type="submit"
      onclick="javascript:bdj.sendCmd('qpclearqueue');"
      name="clearq" value="Clear Queue">
    <hr>
    <select id="playlistsel" name="playlistsel"></select>
    <input class="wb" type="submit"
      onclick="javascript:bdj.sendQPPlaylistCmd('qpplaylist');"
      name="plclearplay" value="Clear & Play">
    <input class="wb" type="submit"
      onclick="javascript:bdj.sendQPPlaylistCmd('qpplaylistqueue');"
      name="plqueue" value="Queue">
  </div>
  <script type="text/javascript">
var bdj = {};

bdj.updIntervalId = '';
bdj.chgInUse = false;
bdj.currentPage = 1;
bdj.maxPages = 2;
bdj.speedIntervalId = '';
bdj.volumeIntervalId = '';
bdj.muteState = 0;

bdj.chgStart = function () {
  bdj.chgInUse = true;
}
bdj.chgEnd = function () {
  bdj.chgInUse = false;
}
bdj.chgMute = function () {
  bdj.muteState = 1-bdj.muteState;
  var ob = document.getElementById("mutei");
  if (bdj.muteState == 0) {
    ob.src = 'mrc/light/mute.svg';
  } else {
    ob.src = 'mrc/light/unmute.svg';
  }
  bdj.sendCmd ('volmute');
}
bdj.chgVolume = function () {
  var oi = document.getElementById("volumeb");
  var o = document.getElementById("vold");
  o.innerHTML = oi.value.toString()+'%';
  if (bdj.IntervalId != '') {
    clearInterval (bdj.volumeIntervalId);
  }
  bdj.volumeIntervalId = setInterval(bdj._chgVolume,100);
}

bdj._chgVolume = function () {
  var o = document.getElementById("volumeb");
  bdj.sendCmd ('volume '+o.value);
  clearInterval (bdj.volumeIntervalId);
  bdj.volumeIntervalId = '';
}

bdj.chgSpeed = function () {
  var oi = document.getElementById("speedb");
  var o = document.getElementById("speedd");
  o.innerHTML = oi.value.toString()+'%';
  if (bdj.speedIntervalId != '') {
    clearInterval (bdj.speedIntervalId);
  }
  bdj.speedIntervalId = setInterval(bdj._chgSpeed,100);
}

bdj._chgSpeed = function () {
  var o = document.getElementById("speedb");
  bdj.sendCmd ('speed '+o.value);
  clearInterval (bdj.speedIntervalId);
  bdj.speedIntervalId = '';
}

bdj.nextPage = function () {
  var p = "controlspage"+bdj.currentPage;
  var o = document.getElementById (p);
  if ( o ) {
    o.className = "divnone";
  }
  ++bdj.currentPage;
  if (bdj.currentPage > bdj.maxPages) {
    bdj.currentPage = 1;
  }
  p = "controlspage"+bdj.currentPage;
  o = document.getElementById (p);
  if ( o ) {
    o.className = "divcenter";
  }
}

bdj.sendQuickplayCmd = function (cmd) {
  var o = document.getElementById("dancelist");
  bdj.sendCmd (cmd + ' {' + o.options[o.selectedIndex].value + '}');
}

bdj.sendQPPlaylistCmd = function (cmd) {
  var o = document.getElementById("playlistsel");
  bdj.sendCmd (cmd + ' {' + o.options[o.selectedIndex].value + '}');
}

bdj.sendCmd = function (cmd) {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      // sending a command will get the same response back as bdjupdate
      bdj.updateData (xhr.responseText);
    }
  };
  xhr.open('GET', '/cmd?'+cmd, true);
  xhr.send();
}

bdj.updateDanceList = function (data) {
  var o = document.getElementById("dancelist");
  if (o) {
    o.innerHTML = data;
  }
}

bdj.updatePlayListSel = function (data) {
  var o = document.getElementById("playlistsel");
  if (o) {
    o.innerHTML = data;
  }
}

bdj.updateData = function (data) {
  var jd = JSON.parse (data);
  if (! jd) {
    return;
  }
  if (jd.playstate) {
    var o = document.getElementById("playstatusi");
    o.src = 'mrc/light/'+jd.playstate+".svg";
  }

  o = document.getElementById("pausestatusd");
  var ob = document.getElementById("pauseblankd");
  if (jd.willpause == "true") {
    o.className = "divinline";
    ob.className = "divnone";
  } else {
    o.className = "divnone";
    ob.className = "divinline";
  }

  o = document.getElementById("repeatstatusd");
  var ob = document.getElementById("repeatblankd");
  if (jd.repeat == "true") {
    o.className = "divinline";
    ob.className = "divnone";
  } else {
    o.className = "divnone";
    ob.className = "divinline";
  }

  o = document.getElementById("pauseatendi");
  o.src = "led_off.svg";
  if (jd.pauseatend == "true") {
    o.src = "led_on.svg";
  }

  if (jd.vol) {
    o = document.getElementById("vold");
    o.innerHTML = jd.vol;
    o = document.getElementById("volumeb");
    jd.vol = jd.vol.replace(new RegExp("%$"), "").trim();
    o.value = jd.vol;
  }

  if (jd.speed) {
    o = document.getElementById("speedd");
    o.innerHTML = jd.speed;
    o = document.getElementById("speedb");
    jd.speed = jd.speed.replace(new RegExp("%$"), "").trim();
    o.value = jd.speed;
  }

  o = document.getElementById("dance");
  o.innerHTML = jd.dance;
  o = document.getElementById("artist");
  o.innerHTML = jd.artist;
  o = document.getElementById("title");
  o.innerHTML = jd.title;
}
bdj.getDanceList = function () {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      bdj.updateDanceList (xhr.responseText);
    }
  };
  xhr.open('GET', '/getdancelist', true);
  xhr.send();
}
bdj.getPlayListSel = function () {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      bdj.updatePlayListSel (xhr.responseText);
    }
  };
  xhr.open('GET', '/getplaylistsel', true);
  xhr.send();
}
bdj.doUpdate = function () {
  if (bdj.chgInUse) {
    // don't run an update if the user is currently messing with
    // one of the sliders.
    return;
  }
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      bdj.updateData (xhr.responseText);
    }
  };
  xhr.open('GET', '/bdjupdate', true);
  xhr.send();
}
bdj.setSize = function (o,s) {
  o.height = s;
  o.width = s;
}
bdj.doLoad = function () {
  var o = document.getElementById("fadeb");
  var s = o.clientHeight;
  o = document.getElementById("muteb");
  bdj.setSize(o,s);
  o = document.getElementById("mutei");
  bdj.setSize(o,s-4);
  o = document.getElementById("pausestatusi");
  bdj.setSize(o,s);
  o = document.getElementById("playstatusi");
  bdj.setSize(o,s);
  o = document.getElementById("repeatstatusi");
  bdj.setSize(o,s);
  o = document.getElementById("pauseatendi");
  bdj.setSize(o,s);
  o = document.getElementById("nextpagei");
  bdj.setSize(o,s);
  bdj.getDanceList();
  bdj.getPlayListSel();
  bdj.doUpdate();
  updIntervalId = setInterval(bdj.doUpdate,500);
}

window.onload = function () { bdj.doLoad(); };
window.onresize = function () { bdj.doLoad(); };
  </script>
</body>
</html>
