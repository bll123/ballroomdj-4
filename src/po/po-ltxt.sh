#!/bin/bash
#
# Copyright 2024 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src/po
cwd=$(pwd)

TMPLDIR=../../templates
LOCALEDATA=${TMPLDIR}/localization.txt
keycount=0

function appendlocaledata {
  keycount=$1
  locale=$2
  iso639_2=$3
  langdesc=$4

  slocale=$(echo $locale | sed 's,\(..\).*,\1,')
  pofile=po/${locale}.po

  for txt in standardrounds queuedance; do
    ttxt=$txt
    if [[ $ttxt == queuedance ]]; then ttxt="QueueDance"; fi
    xl=$(sed -n "\~msgid \"${ttxt}\"$~ {n;p;}" $pofile)
    xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,')
    if [[ $xl == "" ]]; then
      xl=$ttxt
    fi
    eval $txt="\"$xl\""
  done

  echo KEY >> $LOCALEDATA
  echo "..$keycount" >> $LOCALEDATA
  echo ISO639_2 >> $LOCALEDATA
  echo "..${iso639_2}" >> $LOCALEDATA
  echo LONG >> $LOCALEDATA
  echo "..$locale" >> $LOCALEDATA
  echo SHORT >> $LOCALEDATA
  echo "..$slocale" >> $LOCALEDATA
  echo DISPLAY >> $LOCALEDATA
  echo "..$langdesc" >> $LOCALEDATA
  echo STDROUNDS >> $LOCALEDATA
  echo "..${standardrounds}" >> $LOCALEDATA
  echo QDANCE >> $LOCALEDATA
  echo "..${queuedance}" >> $LOCALEDATA
}

> $LOCALEDATA
echo "# localization" >> $LOCALEDATA
echo "# generated $(date +%F)" >> $LOCALEDATA

# if there are any changes to complete.txt,
# this loop is duplicated in pobuild.sh
while read line; do
  case $line in
    '#'*)
      continue
      ;;
    '')
      continue
      ;;
  esac

  set $line
  locale=$1
  tmpllocale=$2
  weblocale=$3
  iso639_2=$4
  englishnm=$5
  shift
  shift
  shift
  shift
  langdesc="$*"

  appendlocaledata ${keycount} ${locale} ${iso639_2} "${langdesc}"

  keycount=$(($keycount + 1))

done < complete.txt

exit 0
