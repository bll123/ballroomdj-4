#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src/po
cwd=$(pwd)

WEBDIR=../../web
BASEFN=bdj4.html
WEBSITE=${WEBDIR}/${BASEFN}.en

html=$(cat $WEBSITE | tr '\n' '@' |
    sed -e 's,\([A-Za-z0-9 .]\) *@ *\([A-Za-z0-9 .]\),\1 \2,g')

for pofile in web-*.po; do
  if [[ $pofile == web-en_GB.po ]]; then
    continue
  fi

  set -o noglob

  sedcmd=""
  while read -r line; do
    case $line in
      "msgid \"\"")
        continue
        ;;
      msgid*)
        ;;
      msgstr*)
        continue
        ;;
      *)
        continue
        ;;
    esac
    nl=$(printf "%s" "$line" |
      sed -e 's,^msgid ",,' -e 's,"$,,')
    xl=$(sed -n "\~msgid \"${nl}\"$~ {n;p;}" $pofile)
    case $xl in
      ""|msgstr\ \"\")
        continue
        ;;
    esac
    xl=$(printf "%s" "$xl" | sed -e 's,^msgstr ",,' -e 's,"$,,' -e 's,\&,\\&amp;,g' -e "s,',!!!,g")

    sedcmd+="-e 's~\"${nl}\"~\"${xl}\"~g' "
    sedcmd+="-e 's~>${nl}<~>${xl}<~g' "
  done < $pofile

  locale=$(printf "%s" ${pofile} | sed -e 's,web-\([^.]*\)\.po,\1,')
  lang=$(printf "%s" ${locale} | sed -e 's,\(..\).*,\1,')
  sedcmd+="-e 's~lang=\"[^\"]*\"~lang=\"${lang}\"~' "
  nfn=${WEBDIR}/${BASEFN}.${lang}
  nhtml=$(printf "%s" "$html" | eval sed ${sedcmd})
  nnhtml=$(printf "%s" "$nhtml" | tr '@' '\n')
  printf "%s" "$nnhtml" | sed -e "s~!!!~'~g" > $nfn
  set +o noglob
done

exit 0
