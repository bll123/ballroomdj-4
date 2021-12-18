#!/usr/bin/awk
#
# Copyright 2010-2018 Brad Lanam Walnut Creek CA
# Copyright 2020 Brad Lanam Pleasant Hill CA

BEGIN {
  dclstart = "[	 *]" ARGV[2] "[	 ]*\\(";
  delete ARGV[2];
  bcount = 0;
  acount = 0;
  ins = 0;
  havestart = 0;
  doend = 0;
  sarr[0] = "";
#print "dclstart:" dclstart;
}

{
#print "havestart:" havestart " ins:" ins " bcount:" bcount " acount:" acount;
  if ($0 ~ /^#/) {
    next;
  } else if (ins == 0 && $0 ~ dclstart) {
#print "start: " $0;
    ins = 1;
    acount = 0;
    sarr[acount] = $0;
    acount = acount + 1;
    havestart = 1;
    tstr = $0;
    gsub (/[^\(]/, "", tstr);
    bcount = bcount + length (tstr);
#print "start: " length(tstr) " (";
    tstr = $0;
    gsub (/[^\)]/, "", tstr);
    bcount = bcount - length (tstr);
#print "start: " length(tstr) " )";
    if (bcount <= 0 && length (tstr) > 0) {
      doend = 1;
    }
  } else if (ins == 1 && $0 ~ /\(/) {
#print "(: " $0;
    sarr[acount] = $0;
    acount = acount + 1;
    tstr = $0;
    gsub (/[^\(]/, "", tstr);
    bcount = bcount + length (tstr);
#print "(: " length(tstr) " (";
    tstr = $0;
    gsub (/[^\(]/, "", tstr);
    bcount = bcount - length (tstr);
#print "(: " length(tstr) " )";
    if (bcount <= 0) {
      ins = 0;
    }
  } else if (ins == 1 && $0 ~ /\)/) {
#print "): " $0;
    sarr[acount] = $0;
    acount = acount + 1;
    tstr = $0;
    gsub (/[^\)]/, "", tstr);
    bcount = bcount - length (tstr);
#print "): " length(tstr) " )";
    if (bcount <= 0) {
      if (havestart == 1) {
        doend = 1;
      }
      ins = 0;
    }
  } else if (ins == 1) {
#print "1: " $0;
    sarr[acount] = $0;
    acount = acount + 1;
  }

  if (doend == 1) {
    for (i = 0; i < acount; ++i) {
      print sarr[i];
    }
    exit;
  }
}
