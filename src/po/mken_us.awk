#!/usr/bin/awk

{
  l = $0;
  gsub (/== English .GB/, "== English (US", l);
  gsub (/english\/gb/, "english/us", l);
  l = gensub (/([Cc])olour/, "\\1olor", "g", l);
  l = gensub (/([Oo])rganis/, "\\1rganiz", "g", l);
  l = gensub (/([Cc])entimetre/, "\\1entimeter", "g", l);
  gsub (/LICENCE/, "LICENSE", l);
  print l;
}

