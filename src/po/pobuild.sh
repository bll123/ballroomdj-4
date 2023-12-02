#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#
# This is the main po creation script.
#
# a) Run the main makefile to update bdj4.pot, en_GB.po, en_US.po
#     poexttmpl.sh (potemplates.c)
#     poextract.sh (bdj4.pot)
#       extract-helptext.awk
#     po-en-gb.sh (po/en_GB.po)
#       po-mk.sh
#         lang-lookup.sh
#           lang-lookup.awk
#     po-en-us.sh (po/en_US.po)
#       mken_us.awk
# b) Create the other language .po files
#     Makefile-po
#       po-mk.sh
#         lang-lookup.sh
#           lang-lookup.awk
# c) Update the .mo files
#     Makefile-inst
# d) Create the localization.txt file while processing the locales.
# e) Update all of the templates/<locale> directories.
#     Makefile-tmpl
#       po-tmpl.sh
#       po-html.sh
# f) Update all the web files
#     Makefile-web
#       po-instweb.sh
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src/po
cwd=$(pwd)

# during this process, create the localization.txt file
# set up the stuff that is needed

TMPLDIR=../../templates
LOCALEDIR=../../locale

export keycount=0

function appendlocaledata {
  pofile=$1
  locale=$2
  slocale=$3
  iso6392=$4
  langdesc=$5

  for txt in automatic standardrounds queuedance; do
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
  echo LONG >> $LOCALEDATA
  echo "..$locale" >> $LOCALEDATA
  echo SHORT >> $LOCALEDATA
  echo "..$slocale" >> $LOCALEDATA
  echo ISO639-2 >> $LOCALEDATA
  echo "..$iso6392" >> $LOCALEDATA
  echo DISPLAY >> $LOCALEDATA
  echo "..$langdesc" >> $LOCALEDATA
  echo AUTO >> $LOCALEDATA
  echo "..${automatic}" >> $LOCALEDATA
  echo STDROUNDS >> $LOCALEDATA
  echo "..${standardrounds}" >> $LOCALEDATA
  echo QDANCE >> $LOCALEDATA
  echo "..${queuedance}" >> $LOCALEDATA

  keycount=$(($keycount+1))
}

LOCALEDATA=${TMPLDIR}/localization.txt
> $LOCALEDATA
echo "# localization" >> $LOCALEDATA

# first, make bdj4.pot, en_GB.po, en_US.po
make

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

  iso6392=$2
  englishnm=$3
  shift
  shift
  shift
  langdesc="$*"

  slocale=$(echo $locale | sed 's,\(..\).*,\1,')

  # create the .po file

  # The .po files for en_GB and en_US are already created
  if [[ $locale != en_GB && $locale != en_US ]]; then
    make -f Makefile-po \
        LOCALE=${locale} \
        SLOCALE=${slocale} \
        ENGNM=${englishnm} \
        LANGDESC="${langdesc}"
  fi

  # re-build the .mo file

  make -f Makefile-inst \
        LOCALE=${locale} \
        SLOCALE=${slocale}

  # create the short-locale links to the long-locale
  # en -> en_GB, not en_US.
  if [[ ${locale} != en_US ]]; then
    (
      cd ${LOCALEDIR}
      # do not replace existing links
      test -h ${slocale} || ln -s ${locale} ${slocale}
    )
  fi

  # add this entry to the localization.txt file

  pofile=po/${locale}.po

  appendlocaledata ${pofile} ${locale} ${slocale} ${iso6392} "${langdesc}"

  # create the localized template files

  if [[ $locale == en_GB || $locale == en_US ]]; then
    # en_US has its own localized templates
    # en_GB/en_US web page is the base
    continue
  fi

  make -f Makefile-tmpl \
      LOCALE=${locale} \
      SLOCALE=${slocale} \
      LANGDESC="${langdesc}"

  if [[ $slocale == pl ]]; then
    slocale=po
  fi
  make -f Makefile-web \
      LOCALE=${locale} \
      SLOCALE=${slocale}

done < complete.txt
