#!/usr/bin/gawk -f
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

function processitem() {
# print "## -- p: line: " $0;
  if ($0 ~ /^$/) {
# if (! start) { print "## -- p: start"; }
# if (instr) { print "## -- p: msgstr: " tmstr; }
    if (! start && doprint) {
      print "";
    }
    start = 1;
    return;
  }
  if (start == 1 && $0 ~ /^msgid/) {
    inid = 1;
    instr = 0;
    tidline = $0;
    tmid = $0;
    sub (/^msgid */, "", tmid);
    if (tmid == "\"\"") {
      tmid = "";
    }
    next;
  }
  if (start == 1 && $0 ~ /^msgstr/) {
# print "## -- p: msgid: " tmid;
    inid = 0;
    instr = 1;
    tmp = $0;
    sub (/^msgstr */, "", tmp);
    tmstr = tmp;
    next;
  }
  if (start == 1 && $0 ~ /^"/) {
    if (inid) {
      tidline = tidline ORS $0;
      if (tmid == "") {
        tmid = $0;
      } else {
        tmid = tmid ORS $0;
      }
    }
    if (instr) {
      tmp = $0;
      tmstr = tmstr ORS tmp;
    }
    next;
  }

  if (doprint) {
# print "## -- print = " $0;
    print $0;
    instr = 0;
    inid = 0;
  }
}

function dumpmsg() {
# print "## -- nidline = " nidline;
# print "## -- nmid = " nmid;
# print "## -- msgstr = " nstrline;
# print "## -- olddata = /" olddata [nmid] "/";
  if (nstrline != "\"\"" && nmid !~ /helptext_/) {
# print "## -- already present, not helptext"
    print nidline;
    printf "msgstr ";
    print nstrline;
  } else if (olddata [nmid] != "") {
# print "## -- not empty";
    print nidline;
    printf "msgstr ";
    print olddata [nmid];
  } else {
    sub (/[:.]"$/, "\"", nmid);
    if (nstrline == "\"\"" && olddata [nmid] != "") {
# print "## -- not empty w/o period";
      print nidline;
      printf "msgstr ";
      print olddata [nmid];
    } else {
      nmid = gensub (/([Cc])olour/, "\\1olor", "g", nmid);
      nmid = gensub (/([Oo])rganis/, "\\1rganiz", "g", nmid);
      nmid = gensub (/([Cc])entimetre/, "\\1entimeter", "g", nmid);
      if (nstrline == "\"\"" && olddata [nmid] != "") {
# print "## -- not empty alt spelling";
        print nidline;
        printf "msgstr ";
        print olddata [nmid];
      } else {
# print "## -- is empty";
        print nidline;
        printf "msgstr ";
        print nstrline;
      }
    }
  }

  print "";
  instr = 0;
  inid = 0;
}

BEGIN {
  state = 0;
}

{
  if (FNR == 1) {
    state++;
    start = 0;
    inid = 0;
    instr = 0;
  }
  if ($0 ~ /^## -- /) {
    # skip debug comments
    next;
  }
  if (state == 1) {
    doprint = 0;
    processitem();
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
  if (state == 2) {
    doprint = 1;
    processitem();
    if (start && instr) {
      nmid = tmid;
      nidline = tidline;
      nstrline = tmstr;
      dumpmsg();
    }
  }
}

END {
  if (doprint && start && instr) {
    dumpmsg();
  }
}
