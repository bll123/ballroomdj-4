#!/usr/bin/awk
#
# Copyright 2010-2018 Brad Lanam Walnut Creek CA
# Copyright 2020 Brad Lanam Pleasant Hill CA

BEGIN {
  if (dcode != "T") {
    dcode = "F";
  }
  ststructA = "(struct|class|union|enum)[	 ]*";
  ststruct1 = "(struct|class|union|enum)[	 ]*\\{";
  ststruct2 = "typedef[	 ][	 ]*(struct|class|union|enum)[ 	]*[a-zA-Z0-9_]*[ 	]*{?[ 	]*$";
  ststart1 = "(struct|class|union|enum)[	 ]*" ARGV[2] "$";
  ststart2 = "(struct|class|union|enum)[	 ]*" ARGV[2] "[^a-zA-Z0-9_]";
  stforward = "(struct|class|union|enum)[	 ]*" ARGV[2] "[	 ]*;";
  stother = "(struct|class|union|enum)[	 ]*" ARGV[2] "[ 	][	 ]*[*a-zA-Z0-9_]";
  stend = "[	 *]" ARGV[2] "_t[ 	]*;";
  stendb = "[	 *]" ARGV[2] "[ 	]*;";
  delete ARGV[2];
  bcount = 0;
  acount = 0;
  ins = 0;
  inend = 0;
  havestart = 0;
  doend = 0;
  hadend = 0;
  sarr[0] = "";
  nsarr[0] = "";
  savens = "";
  lineno = 0;
#print "dcode:" dcode;
#print "ststruct1:" ststruct1;
#print "ststruct2:" ststruct2;
#print "ststart1:" ststart1;
#print "ststart2:" ststart2;
#print "stforward:" stforward;
#print "stother:" stother;
#print "stend:" stend;
#print "stendb:" stendb;
}

{
  lineno = lineno + 1;
#print lineno ": " $0;
#if ($0 ~ ststruct1) { print "  " lineno ":   matches ststruct1"; }
#if ($0 ~ ststruct2) { print "  " lineno ":   matches ststruct2"; }
#if ($0 ~ ststart1) { print "  " lineno ":   matches ststart"; }
#if ($0 ~ ststart2) { print "  " lineno ":   matches ststart"; }
#if ($0 ~ stforward) { print "  " lineno ":   matches stforward"; }
#if ($0 ~ stother) { print "  " lineno ":   matches stother: " $0; }
#if ($0 ~ stend) { print "  " lineno ":   matches stend: " $0; }
#if ($0 ~ stendb) { print "  " lineno ":   matches stendb: " $0; }
#print "havestart:" havestart ":ins:" ins ":bcount:" bcount ":hadend:" hadend ":";
  if ($0 ~ /^#/) {
    next;
  } else if (ins == 0 && ($0 ~ ststart1 || $0 ~ ststart2) && $0 !~ stforward && $0 !~ stother) {
#print lineno ":   start: ";
    hadend = 0;
    savens = "";
    for (val in nsarr) { delete nsarr[val]; }
    nsarr[0] = "";
    acount = 0;

    ins = 1;
    gsub (/[	 ]*$/, "", $0);
    sarr[acount] = $0;
    acount = acount + 1;
    havestart = 1;
    tstr = $0;
    gsub (/[^\{]/, "", tstr);
    bcount = bcount + length (tstr);
    tstr = $0;
    gsub (/[^}]/, "", tstr);
    bcount = bcount - length (tstr);
    if (bcount <= 0 && length (tstr) > 0) {
      doend = 1;
    }
  } else if (havestart == 0 && bcount == 0 &&
      ($0 ~ ststruct1 || $0 ~ ststruct2) &&
      $0 !~ stforward && $0 !~ stother) {
#print lineno ":   struct: ";
    hadend = 0;
    savens = "";
    for (val in nsarr) { delete nsarr[val]; }
    nsarr[0] = "";
    acount = 0;

    ins = 1;
    tstr = $0;
    gsub (/[^\{]/, "", tstr);
    bcount = bcount + length (tstr);
    tstr = $0;
    gsub (/[^}]/, "", tstr);
    bcount = bcount - length (tstr);
    if (bcount <= 0 && length(tstr) > 0) {
      ins = 0;
      bcount = 0;
      savens = "";
      for (val in nsarr) { delete nsarr[val]; }
      nsarr[0] = "";
    }
    if ($0 ~ stend || $0 ~ stendb) {
#print lineno ":   matches endA: doend";
      doend = 1;
    }
    acount = 0;
    gsub (/[	 ]*$/, "", $0);
    sarr[acount] = $0;
    acount = acount + 1;
  } else if (hadend == 1 && acount > 0 && bcount == 0 && havestart == 0 && ($0 ~ stend || $0 ~ stendb)) {
#print lineno ":   endB: ";
    hadend = 0;
    gsub (/[	 ]*$/, "", $0);
    sarr[acount] = $0;
    acount = acount + 1;
    doend = 1;
  } else if (ins == 1 &&
      $0 !~ /(struct|class|union|enum)[	 ]*\{/ &&
      $0 ~ /(const *)?(struct|class|union|enum)[	 ]*[a-zA-Z_][a-zA-Z0-9_]*[ 	\{]*$/ &&
      $0 !~ /(const *)?(struct|class|union|enum)[	 ].*;/) {
# nested structure, not an unnamed structure
# is named, but not followed by anything else
#print lineno ":   nested struct: ";
    hadend = 0;
    savens = "";
    gsub (/[	 ]*$/, "", $0);
    tstr = $0;
    if (dcode) {
      if ($0 ~ /struct/) {
        ttype = "struct";
        tlab = "C_ST_";
#print lineno ":  found struct"
      }
      if ($0 ~ /class/) {
        ttype = "class";
        tlab = "C_CLASS_";
#print lineno ":  found class"
      }
      if ($0 ~ /union/) {
        ttype = "union";
        tlab = "C_UN_";
#print lineno ":  found union"
      }
      if ($0 ~ /enum/) {
        ttype = "enum";
        tlab = "C_ENUM_";
#print lineno ":  found enum"
      }
      sub (/(struct|class|union|enum)[ 	]*/, "&" tlab, tstr);
#print lineno ":  nested(A): " tstr
    }
    sarr [acount] = tstr;
    acount = acount + 1;
    tstr = $0;
    gsub (/[^\{]/, "", tstr);
    bcount = bcount + length (tstr);
    tstr = $0;
    gsub (/[^}]/, "", tstr);
    bcount = bcount - length (tstr);
    if (bcount <= 0 && length(tstr) > 0) {
      ins = 0;
    }
    if (ins == 1) {
      tstr = $0;
#print lineno ":   nested(B): ", $0
      if (dcode) {
        sub (/[	 const]*(struct|class|union|enum)[	 ]*/, "", tstr);
        sub (/[	 ].*/, "", tstr);
      }
#print lineno ":   nested(C): ", tstr;
      savens = tstr;
      if (bcount > 1) {
        nsarr[bcount] = tstr;
        savens = "";
#print lineno ":   nested save: " tstr;
      }
    }
  } else if (ins == 1 && $0 ~ /\{/) {
#print lineno ":   {: ";
    hadend = 0;
    sarr[acount] = $0;
    acount = acount + 1;
    tstr = $0;
    gsub (/[^\{]/, "", tstr);
    bcount = bcount + length (tstr);
    tstr = $0;
    gsub (/[^}]/, "", tstr);
    bcount = bcount - length (tstr);
    if (bcount <= 0 && length(tstr) > 0) {
      ins = 0;
    }
    if (ins == 1 && bcount > 1 && savens != "") {
#print lineno ":   nested last: ", savens;
      if (bcount > 1) {
        nsarr[bcount] = savens;
#print lineno ":   nested save: " savens;
      }
      savens = "";
    }
  } else if (ins == 1 && $0 ~ /}/) {
#print lineno ":   }: ";
    gsub (/[	 ]*$/, "", $0);
    sarr[acount] = $0;
    acount = acount + 1;
    tstr = $0;
    gsub (/[^}]/, "", tstr);
    l = length(tstr);
    bcount = bcount - l;
    stendtestb = "}[	 *][a-zA-Z0-9_]*[ 	]*;";
    if (l > 0 && $0 !~ stendtestb) {
#print lineno ":   }: hadend: =1: ";
      hadend = 1;
    } else {
#print lineno ":   }: hadend: =0: ";
      hadend = 0;
    }
    if (bcount <= 0) {
#print lineno ":   }: bcount: 0";
      bcount = 0;
      if (havestart == 1) {
#print lineno ":   }: havestart: doend";
        doend = 1;
      }
      if ($0 ~ stend || $0 ~ stendb) {
#print lineno ":   matches endC: doend";
        doend = 1;
      }
      ins = 0;
    } else if (l > 0 && $0 !~ /}[	 ;]*$/) {
#print lineno ":   end struct dcl: " $0;
      if (dcode) {
        tstr = $0;
# if there's a declaration of a pointer to the nested structure
# we'll be adding the declaration.
# if so, remove the name now.
# otherwise, keep the name for processing by d-main.sh
        if (nsarr[bcount + 1] != "") {
          sub (/}.*/, "};", tstr);
          sarr [acount - 1] = tstr;
        }
#print lineno ":   nsarr1: " nsarr[bcount + 1];
        if (nsarr[bcount + 1] != "") {
          tstr = $0;
          sub (/}[	 ]*/, tlab nsarr[bcount + 1] " ", tstr);
          sarr [acount] = tstr;
          acount = acount + 1;
        }
      }
      hadend = 0;
    }
  } else if (ins == 1) {
#print lineno ":   1: " $0;
#print lineno ":   1: hadend:" hadend;
    if (hadend == 1 && nsarr[bcount + 1] != "") {
      if ($0 ~ /[	 *]*[_A-Za-z]/) {
#print lineno ":   1: hadend: match"
        tstr = $0;
        if (dcode) {
          tstr = " " tlab nsarr[bcount + 1] " " $0;
        }
        sarr[acount] = tstr;
        acount = acount + 1;
      } else {
        gsub (/[	 ]*$/, "", $0);
        sarr[acount] = $0;
        acount = acount + 1;
      }
    } else {
      gsub (/[	 ]*$/, "", $0);
      sarr[acount] = $0;
      acount = acount + 1;
    }
    hadend = 0;
  } else if (ins == 0) {
    if ($0 ~ ststructA) {
      hadend = 0;
    }
  }

  if (doend == 1) {
    for (i = 0; i < acount; ++i) {
      print sarr[i];
    }
    exit;
  }
}
