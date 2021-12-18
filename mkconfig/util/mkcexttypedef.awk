#!/usr/bin/awk
#
# Copyright 2010-2018 Brad Lanam Walnut Creek CA
# Copyright 2020 Brad Lanam Pleasant Hill CA

BEGIN {
  tdstart = "[ 	]*typedef";
  funcpat = "[ 	(*]" ARGV[2] "[ 	)]";
  semipat = "[ 	*]" ARGV[2] "[ 	]*;$";
  delete ARGV[2];
  intypedef = 0;
  acount = 0;
  docheck = 0;
  tarr = "";
  lineno = 0;
#print "tdstart:" tdstart ":";
#print "funcpat:" funcpat ":";
#print "semipat:" semipat ":";
}

{
  lineno = lineno + 1;
#print lineno ": " $0;

  if ($0 ~ /^#/) {
    next;
  } else if (intypedef == 0 && $0 ~ tdstart) {
#print "start:";
    intypedef = 1;
    acount = 0;
    sarr[acount] = $0;
    acount = acount + 1;
    tarr = tarr $0;
    if ($0 ~ /;$/) {
#print "have semi";
      docheck = 1;
    }
  } else if (intypedef == 1) {
#print "in:";
    sarr[acount] = $0;
    acount = acount + 1;
    tarr = tarr $0;
    if ($0 ~ /;$/) {
#print "have semi";
      docheck = 1;
    }
  }

  if (docheck == 1) {
#print "docheck: tarr:" tarr ":";
    if (tarr ~ funcpat || tarr ~ semipat) {
#print "docheck: match";
      doend = 1;
    }
  }

  if (doend == 1) {
    for (i = 0; i < acount; ++i) {
      print sarr[i];
    }
    exit;
  }

  if (docheck == 1) {
#print "docheck: no match";
    acount = 0;
    tarr = "";
    docheck = 0;
    intypedef = 0;
  }
}
