#!/bin/bash
#
#

cwd=$(pwd)
case ${cwd} in
  */bdj4)
    ;;
  */src)
    cd ..
    ;;
  */utils)
    cd ../..
    ;;
esac

OUT=templates/html-list.txt
OUTN=${OUT}.n

for fn in $(echo templates/*.html*); do
  case $fn in
    *mobilemq.html|*qrcode.html)
      continue
      ;;
  esac

  title=$(sed -n -e 's,<!-- ,,' -e 's, -->,,' -e '1,1 p' $fn)
  echo $title >> $OUTN
  echo "..$(basename $fn)" >> $OUTN
done
mv -f $OUTN $OUT
exit 0
