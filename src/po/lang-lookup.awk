#!/usr/bin/gawk -f
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#

function processitem() {
  print "## -- p" state ": process " doprocess "/" inid "/" instr ": /" $0 "/" >> DBGFILE;
  if ($0 ~ /^$/) {
    if (state == 2 && instr) {
      nmid = tmid;
      nidline = tidline;
      nstrline = tmstr;
      dumpmsg();
    }
    doprocess = 1;
    inid = 0;
    instr = 0;
    return;
  }
  if (doprocess == 1 && $0 ~ /^msgid/) {
    inid = 1;
    instr = 0;
    tidline = $0;
    tmid = $0;
    sub (/^msgid */, "", tmid);
    if (tmid == "\"\"") {
      tmid = "";
    }
    print "## -- p" state ": msgid: " tmid >> DBGFILE;
    next;
  }
  if (doprocess == 1 && $0 ~ /^msgstr/) {
    inid = 0;
    instr = 1;
    tmp = $0;
    sub (/^msgstr */, "", tmp);
    tmstr = tmp;
    print "## -- p" state ": msgstr: " tmstr >> DBGFILE;
    next;
  }
  if (doprocess == 1 && $0 ~ /^"/) {
    if (inid) {
      tidline = tidline ORS $0;
      if (tmid == "") {
        tmid = $0;
      } else {
        tmid = tmid ORS $0;
      }
      print "## -- p" state ": msgid: cont: " tmid >> DBGFILE;
    }
    if (instr) {
      tmp = $0;
      tmstr = tmstr ORS tmp;
      print "## -- p" state ": msgstr: cont: " tmstr >> DBGFILE;
    }
    next;
  }
}

function printmsg() {
  print nidline;
  printf "msgstr ";
  print pstrline;
  print nidline >> DBGFILE;
  printf "msgstr " >> DBGFILE;
  print pstrline >> DBGFILE;
}

function saveolddata() {
  print "## -- save old data " tmid >> DBGFILE;
  omid = tmid;
  # always remove colons
  sub (/:"$/, "\"", omid);
  sub (/:"$/, "\"", tmstr);
  if (tmstr == "\"\"") {
    tmstr = "";
  }
  olddata [omid] = tmstr;
  # if the string has a period, create a second entry w/o the period.
  sub (/\."$/, "\"", tmid);
  if (tmid != omid) {
    sub (/\."$/, "\"", tmstr);
    olddata [tmid] = tmstr;
  }
}

function dumpmsg() {
  print "## -- nidline = " nidline >> DBGFILE;
  print "## -- nmid = " nmid >> DBGFILE;
  print "## -- msgstr = " nstrline >> DBGFILE;
  # for the helptext_* items, prefer the data from the current file
  if (nstrline != "\"\"" && nmid !~ /helptext_/) {
    print "## -- already present" >> DBGFILE;
    pstrline = nstrline;
    printmsg();
  } else if (olddata [nmid] != "") {
    print "## -- olddata not empty" >> DBGFILE;
    print "## -- olddata = /" olddata [nmid] "/" >> DBGFILE;
    pstrline = olddata [nmid];
    printmsg();
  } else {
    if (nmid ~ /"\n"/) {
      tmid = nmid;
      gsub (/"\n"/, "", tmid);
      if (nstrline == "\"\"" && olddata [tmid] != "") {
        print "## -- olddata not empty continuation" >> DBGFILE;
        print "## -- olddata = /" olddata [tmid] "/" >> DBGFILE;
        pstrline = olddata [tmid];
        printmsg();
      }
    } else {
      sub (/[:.]"$/, "\"", nmid);
      if (nstrline == "\"\"" && olddata [nmid] != "") {
        print "## -- olddata not empty w/o period" >> DBGFILE;
        print "## -- olddata = /" olddata [nmid] "/" >> DBGFILE;
        pstrline = olddata [nmid];
        printmsg();
      } else {
        nmid = gensub (/([Cc])olour/, "\\1olor", "g", nmid);
        nmid = gensub (/([Oo])rganis/, "\\1rganiz", "g", nmid);
        nmid = gensub (/([Cc])entimetre/, "\\1entimeter", "g", nmid);
        if (nstrline == "\"\"" && olddata [nmid] != "") {
          print "## -- olddata not empty alt spelling" >> DBGFILE;
          print "## -- olddata = /" olddata [nmid] "/" >> DBGFILE;
          pstrline = olddata [nmid];
          printmsg();
        } else {
          print "## -- is empty" >> DBGFILE;
          pstrline = nstrline;
          printmsg();
        }
      }
    }
  }

  print "";
  instr = 0;
  inid = 0;
}

BEGIN {
  # state 0 : not started
  # state 1 : reading old data
  # state 2 : reading current data
  DBGFILE = "/dev/null";
  # DBGFILE = "/dev/stderr";
  state = 0;
  doprocess = 0;
  inid = 0;
  instr = 0;
  doprint = 0;
}

{
  if (FNR == 1) {
    if (state == 1 && doprocess && instr) {
      saveolddata();
    }

    state++;
    if (state == 1) {
      print "## -- == reading old data " FILENAME >> DBGFILE;
      doprint = 0;
    }
    if (state == 2) {
      print "## -- == reading current data " FILENAME >> DBGFILE;
      doprint = 1;
    }
    doprocess = 0;
    inid = 0;
    instr = 0;
  }
  if ($0 ~ /^## -- /) {
    # skip debug comments
    next;
  }
  if (doprint && $0 ~ /^#/) {
    print $0;
    print $0 >> DBGFILE;
    instr = 0;
    inid = 0;
    next;
  }
  if (doprint && state == 2 && doprocess == 0) {
    print $0;
    print $0 >> DBGFILE;
    if ($0 ~ /^$/) {
      doprocess = 1;
      print "## -- p" state ": start" >> DBGFILE;
    }
    next;
  }
  print "## -- p" state ": line: " $0 >> DBGFILE;
  if (state == 1) {
    processitem();
    saveolddata();
  }
  if (state == 2) {
    # the old data has been read in, process the existing po file.
    processitem();
  }
}

END {
  if (doprint && doprocess && instr) {
    nmid = tmid;
    nidline = tidline;
    nstrline = tmstr;
    dumpmsg();
  }
}
