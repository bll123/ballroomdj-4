#!/bin/bash
#
# Copyright 2023-2026 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src/po
cwd=$(pwd)

WEBDIR=../../web
BASEFN=bdj4.html
WEBSITE=${WEBDIR}/${BASEFN}.en

pofile=$1
locale=$2
weblocale=$3

html=$(cat $WEBSITE | tr '\n' '@' |
    sed -e 's,\([A-Za-z0-9 .]\) *@ *\([A-Za-z0-9 .]\),\1 \2,g')

if [[ $locale == en_GB || $locale == en_US ]]; then
  exit 0
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

sedcmd+="-e 's~lang=\"[^\"]*\"~lang=\"${weblocale}\"~' "

nfn=${WEBDIR}/${BASEFN}.${weblocale}
nhtml=$(printf "%s" "$html" | eval sed ${sedcmd})
nnhtml=$(printf "%s" "$nhtml" | tr '@' '\n')
printf "%s\n" "$nnhtml" | sed -e "s~!!!~'~g" > $nfn
set +o noglob

exit 0
