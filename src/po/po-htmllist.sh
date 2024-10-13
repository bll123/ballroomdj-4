#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src/po
cwd=$(pwd)

TMPLDIR=../../templates
HTMLLIST=${TMPLDIR}/html-list.txt
keycount=0

> ${HTMLLIST}
echo "# html-list" >> $HTMLLIST
echo "# generated $(date +%F)" >> $HTMLLIST

for fn in $(echo ${TMPLDIR}/*.html ${TMPLDIR}/*/*.html); do
  case $fn in
    *mobilemq.html|*qrcode.html)
      continue
      ;;
  esac

  tfn=$(echo $fn | sed -e 's,.*templates/,,')
  locale=$(echo $tfn | sed -e 's,/*[a-zA-Z-]*\.html$,,')
  if [[ $locale == "" ]]; then
    locale=en
  fi
  title=$(sed -n -e 's,<!-- ,,' -e 's, -->,,' -e '1,1 p' $fn)

  echo KEY >> $HTMLLIST
  echo "..$keycount" >> $HTMLLIST
  echo LOCALE >> $HTMLLIST
  echo "..$locale" >> $HTMLLIST
  echo TITLE >> $HTMLLIST
  echo "..$title" >> $HTMLLIST
  echo FILE >> $HTMLLIST
  echo "..$tfn" >> $HTMLLIST

  keycount=$(($keycount + 1))

done

exit 0
