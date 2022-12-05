#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
#
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

OUT=templates/html-list.txt
OUTN=${OUT}.n

for fn in $(echo templates/*.html templates/*/*.html); do
  case $fn in
    *mobilemq.html|*qrcode.html)
      continue
      ;;
  esac

  tfn=$(echo $fn | sed 's,templates/,,')
  title=$(sed -n -e 's,<!-- ,,' -e 's, -->,,' -e '1,1 p' $fn)
  echo $title >> $OUTN
  echo "..$tfn" >> $OUTN
done
mv -f $OUTN $OUT
exit 0
