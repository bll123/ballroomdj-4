#!/usr/bin/gawk

function dumpmsg() {
  if (instr) {
#print "# -- nidline = " nidline;
#print "# -- nmid = " nmid;
#print "# -- msgstr = " $0;
#print "# -- olddata = /" olddata [nmid] "/";
    if (olddata [nmid] == "") {
#print "# -- is empty";
      gsub (/%/, "%%", nidline);
      printf nidline;
      print "";
      gsub (/%/, "%%", nstrline);
      printf nstrline;
      print "";
    } else {
#print "# -- not empty";
      gsub (/%/, "%%", nidline);
      printf nidline;
      print "";
      printf "msgstr ";
      printf olddata [nmid];
      print "";
    }
    instr = 0;
  }
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
  if ($0 ~ /^# -- /) {
    # skip debug comments
    next;
  }
  if (state == 1) {
    if ($0 ~ /^$/) {
      start = 1;
      inid = 0;
      instr = 0;
    }
    if (start == 0) {
      next;
    }
    if ($0 ~ /^msgid/) {
      omid = $0;
      sub (/^msgid */, "", omid);
      inid = 1;
    }
    if ($0 ~ /^msgstr/) {
      inid = 0;
      instr = 1;
      tmp = $0;
      sub (/^msgstr */, "", tmp);
      gsub (/%/, "%%", tmp);
      olddata [omid] = tmp;
      if (omid !~ /helptext/) {
        sub (/[:.]"$/, "\"", omid);
        sub (/[:.]"$/, "\"", tmp);
      }
      olddata [omid] = tmp;
    }
    if ($0 ~ /^"/) {
      if (inid) {
        omid = omid $0;
      }
      if (instr) {
        tmp = $0;
        gsub (/%/, "%%", tmp);
        olddata [omid] = olddata [omid] ORS tmp;
      }
    }
  }
  if (state == 2) {
    if ($0 ~ /^$/) {
      start = 1;
      inid = 0;
      dumpmsg();
    }
    if (start == 0) {
      gsub (/%/, "%%");
      printf $0;
      print "";
      next;
    }
    if ($0 ~ /^msgid/) {
      nidline = $0;
      nmid = $0;
      sub (/^msgid */, "", nmid);
      inid = 1;
      next;
    }
    if ($0 ~ /^msgstr/) {
      inid = 0;
      instr = 1;
      nstrline = $0;
      next;
    }
    if ($0 ~ /^"/) {
      if (inid) {
        nidline = nidline ORS $0;
        nmid = nmid $0;
      }
      if (instr) {
        nstrline = nstrline ORS $0;
      }
      next;
    }

    gsub (/%/, "%%");
    printf $0;
    print "";
  }
}

END {
  dumpmsg();
}


