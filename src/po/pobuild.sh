#!/bin/bash
#
# Copyright 2023-2026 Brad Lanam Pleasant Hill CA
#
# This is the main po creation script.
#
# a) Run the main makefile to update bdj4.pot, en_GB.po, en_US.po
#     Makefile
#       poexttmpl.sh (potemplates.c)
#       poextract.sh (bdj4.pot)
#         extract-helptext.awk
#       po-en-gb.sh (po/en_GB.po)
#         po-mk.sh
#           lang-lookup.sh
#             lang-lookup.awk
#       po-en-us.sh (po/en_US.po)
#         mken_us.awk
# b) Create any .po files for new languages
#     Makefile-po
#       po-mk.sh
# c) Update the .mo files
#     Makefile-inst
# d) Create the localization.txt file while processing the locales.
#     Makefile
#       po-ltxt.sh (templates/localization.txt)
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

# first, make bdj4.pot, en_GB.po, en_US.po
make

# if there are any changes to complete.txt,
# this loop is duplicated in po-ltxt.sh
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
  englishnm=$4
  shift
  shift
  shift
  shift
  langdesc="$*"

  slocale=$(echo $locale | sed 's,\(..\).*,\1,')

  # create the .po file

  # The .po files for en_GB and en_US are already created
  # this is for new languages.
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
        SLOCALE=${slocale} \

  # create the short-locale links to the long-locale
  # en -> en_GB, not en_US.
  if [[ ${locale} != en_US ]]; then
    (
      cd ${LOCALEDIR}
      # do not replace existing links
      test -h ${slocale} || ln -s ${locale} ${slocale}
    )
  fi
  if [[ ${locale} == nb_NO ]]; then
    (
      cd ${LOCALEDIR}
      # do not replace existing links
      test -h nn || ln -s ${locale} nn
    )
  fi

  # create the localized template files

  if [[ $locale == en_GB || $locale == en_US ]]; then
    # en_US has its own localized templates
    #   that do not get generated.
    # en_GB/en_US web page is the base
    continue
  fi

  make -f Makefile-tmpl \
      LOCALE=${locale} \
      TMPLLOCALE=${tmpllocale} \
      LANGDESC="${langdesc}"

  make -f Makefile-web \
      LOCALE=${locale} \
      WEBLOCALE=${weblocale}

  keycount=$(($keycount + 1))

done < complete.txt

# create the localization.txt file

make -f Makefile ltxt
make -f Makefile htmllist

exit 0
