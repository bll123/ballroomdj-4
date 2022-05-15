#!/usr/bin/gawk

BEGIN {
  intext = 0;
  intextkey = 0;
  intitle = 0;
  text = "msgstr \"\"\n";
  while (getline < "../../templates/helpdata.txt") {
    if ($0 ~ /^$/) {
      continue;
    }
    if ($0 ~ /^KEY/) {
      if (intext) {
        helptext[textkey] = text;
        helptitle[textkey] = title;
        helpfound[textkey] = 0;
      }
      intext = 0;
      text = "msgstr \"\"\n";
    }
    if ($0 ~ /^TITLE/) {
      intitle = 1;
    }
    if ($0 ~ /^TEXT/) {
      intextkey = 1;
    }
    if ($0 ~ /^\.\./) {
      if (intitle) {
        title = $0;
        gsub (/^\.\./, "", title);
        intitle = 0;
      }
      if (intextkey) {
        textkey = $0;
        gsub (/^\.\./, "", textkey);
        intextkey = 0;
        intext = 1;
      }
    }
    if ($0 ~ /^#/) {
      if (intext) {
        tline = $0;
        gsub (/^#/, "", tline);
        text = text "\"" tline "\"\n";
      }
    }
  }

  if (intext) {
    helptext[textkey] = text;
    helptitle[textkey] = title;
    helpfound[textkey] = 0;
  }

  found = 0;
  currtextkey = "";
}

{
  if (found == 2) {
    if ($0 ~ /^$/) {
      print helptext[currtextkey];
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

  for (textkey in helptext) {
    pat = "^msgid ." textkey ".$";
    currtextkey = textkey;
    if ($0 ~ pat) {
      found = 1;
      break;
    }
  }
}

END {
  print "";
  for (textkey in helptext) {
    if (! helpfound[textkey]) {
      print "#. CONTEXT: title of a getting started help section";
      print "msgid \"" helptitle[textkey] "\"";
      print "msgstr \"\"";
      print "";
      print "#. CONTEXT: text-key for getting started help section (translate the text, not the key)";
      print "msgid \"" textkey "\"";
      print helptext[textkey];
      print "";
    }
  }
}
