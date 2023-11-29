#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#
# a) Run the main makefile to update bdj4.pot, en_GB.po
# b) Updates all of the templates/<locale> directories.
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src/po
cwd=$(pwd)

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

  if [[ $locale == en_GB ]]; then
    # en-gb has already been created and has no separate templates
    continue
  fi

  iso6392=$2
  englishnm=$3
  shift
  shift
  shift
  langdesc="$*"

continue

  slocale=$(echo $locale | sed 's,\(..\).*,\1,')

  # create the .po file

  if [[ $locale != en_US ]]; then
    # en-us is directly created from en-gb, skip it
    make -f Makefile-po \
        LOCALE=${locale} \
        SLOCALE=${slocale} \
        ENGNM=${englishnm} \
        DESC="${langdesc}"
  fi

  # create the localized template files

  make -f Makefile-tmpl LOCALE=${locale} SLOCALE=${slocale}

done < complete.txt
