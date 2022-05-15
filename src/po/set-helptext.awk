#!/usr/bin/gawk

# variable HK : helptext key
# variable TEXT : replacement msgstr

BEGIN {
  found = 0;
  pat = "^msgid ." HK ".$";
}

{
  if (found == 2) {
    if ($0 ~ /^$/) {
      print TEXT;
    } else {
      # already exists
      print "msgstr \"\"";
    }
    print $0;
    found = 0;
  } else if (found == 1) {
    if ($0 ~ /^msgstr ""/) {
      found = 2;
    } else {
      print $0;
      found = 0;
    }
  } else {
    print $0;
  }

  if ($0 ~ pat) {
    found = 1;
  }
}
