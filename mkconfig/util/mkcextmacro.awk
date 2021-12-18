#!/usr/bin/awk
#
# Copyright 2010-2018 Brad Lanam Walnut Creek CA
# Copyright 2020 Brad Lanam Pleasant Hill CA

BEGIN {
  macrostart = "^#[ 	]*define[ 	]*" ARGV[2] "[ 	(]"
  delete ARGV[2];
  ins = 0;
  doend = 0;
#print "macrostart:" macrostart;

  cf = ENVIRON["CFLAGS"];
#print "cf:" cf ":";
  split (cf,cfa);
  for (cfi in cfa) {
#print "cfi:" cfi ":cfa[cfi]:" cfa[cfi];
    if (cfa[cfi] ~ /^-D/) {
      gsub (/-D/,"",cfa[cfi]);
      if (match (cfa[cfi], "=")) {
        split (cfa[cfi], cfaa, /=/);
        tcfv = cfaa[1];
        if (cfaa[2] > 0) {
#print "cflags:" tcfv ":";
          cflags[tcfv] = 1;
        }
      } else {
#print "cflags:" cfa[cfi] ":";
        cflags[cfa[cfi]] = 1;
      }
    }
  }
  exclude = 0;
}

{
#print NR ": " $0;

# remove comments
  gsub (/\/\/.*$/, "");
  gsub (/\/\*.*\*\//, "");
  gsub (/[ 	]*$/, "");

  if ($0 ~ /^# *ifndef/) {
    gsub (/^# *ifndef/, "#if !");
  }
  if ($0 ~ /^# *ifdef/) {
    gsub (/^# *ifdef/, "#if ");
  }

# this is very simplistic.
# there is no precedence and no handling of parentheses
  if ($0 ~ /^# *if/) {
#print "if:";
    l = $0;
    gsub (/^# *if[a-z]*/, "", l);
    gsub (/defined *\(/, "", l);
    gsub (/\)/, "", l);
    gsub (/!/, " NOT ", l);
    gsub (/\&\&/, " AND ", l);
    gsub (/\|\|/, " OR ", l);
    gsub (/^ */, "", l);
#print "if:l:" l;
    l = l " END";
    split (l, la);
    ecount = 0;
    for (val in ea) { delete ea[val]; }
    rval = 0;
    dopop = 0;
    for (li = 1; la[li] != "END"; ++li) {
#print "if:token:" li ":" la[li];
      if (la[li] == "END") {
        continue;
      } else if (la[li] == "AND" || la[li] == "OR") {
        ea[ecount++] = rval;
        ea[ecount++] = la[li];
        continue;
      } else if (la[li] == "NOT") {
        ea[ecount++] = la[li];
        continue;
      } else {
        val = 0;
        if (la[li] ~ /^[0-9]+$/) {
          val = la[li];
        } else if (la[li] in cflags) {
#print "ifB':la[li]:" la[li] ":cflags[la[li]]:" cflags[la[li]] ":";
          val = 1;
        }
#print "ifB:val:" val;

        if (ecount > 0) {
          --ecount;
          if (ea[ecount] == "AND") { rval = ea[ecount - 1] && val; --ecount; }
          if (ea[ecount] == "OR") { rval = ea[ecount - 1] || val; --ecount; }
          if (ea[ecount] == "NOT") { rval = 1 - val; }
        } else {
          rval = val;
        }
#print "ifA:rval:" rval;
      }
      dopop = 0;
    }

#print "ifEND:rval:" rval;
    if (rval == 0) {
      exclude = 1;
    }
  }
  if ($0 ~ /^# *else/) {
#print "else:";
    exclude = 1 - exclude;
  }
  if ($0 ~ /^# *endif/) {
#print "endif:";
    exclude = 0;
  }

  if (exclude == 1) {
#print "excluding";
    next;
  }

  if ($0 ~ /^# *define/ && (NF == 3 || NF == 4)) {
    l = $0;
    gsub (/^# *define */, "", l);
#print "check define:" l ":";
    split (l, la);
    if (la[2] ~ /^[0-9]+$/) {
#print "define:" la[1] ":" la[2];
      if (la[2] > 0) {
        cflags[la[1]] = 1;
      }
      if (la[1] in cflags) {
        cflags[la[1]] = 1;
      }
    }
  }

#print "main:" $0;
  if (ins == 0 && $0 ~ macrostart) {
#print "start: " $0;
    ins = 1;
    macro = $0;
#print "macroA:" macro;
    doend = 1;
    if ($0 ~ /\\$/) {
      doend = 0;
      gsub (/[	 ]*\\$/, " ", macro);
#print "macroB:" macro;
#print "not end: ";
    }
  } else if (ins == 1) {
#print "cont: " $0;
    macro = macro $0;
#print "macroC:" macro;
    doend = 1;
    if ($0 ~ /\\$/) {
      doend = 0;
      gsub (/[	 ]*\\$/, " ", macro);
#print "macroD:" macro;
#print "not end: ";
    }
  } else if (ins == 1) {
#print "ins = 1; end ";
    doend = 1;
  }

  if (doend == 1) {
    print macro;
    exit;
  }
}
