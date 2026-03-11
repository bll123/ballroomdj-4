#!/bin/bash
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#

# creates a po file from the bdj4.pot file

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src
cwd=$(pwd)

rm -f po/*~ po/po/*~ po/web/*~ po/old/*~ > /dev/null 2>&1

export POTFILE=bdj4.pot

. ../VERSION.txt
export VERSION
dt=$(date '+%F')
export dt

function mkpo {
  out=$1
  locale=$2
  elang=$3
  dlang=$4
  xlator=$5

  if [[ $locale == en_US ]]; then
    # en-us is created directly from en-gb, and should never
    # end up here
    return
  fi

  lang=$(echo $locale | sed 's,\(..\).*,\1,')

  if [[ ! -f ${out} && ${locale} != en_GB ]]; then
    echo "-- $(date +%T) creating $out"
    > ${out}
    # this is not quite right...there are fields
    # that need fixing.
    echo "# == ${dlang}" >> ${out}
    echo "# -- ${elang}" >> ${out}
    sed -n '/^msgid/,/^$/ p' ${POTFILE} >> ${out}
  fi

  if [[ ! -f ${out} || ${locale} == en_GB ]]; then
    echo "-- $(date +%T) creating $out"
    # new file; $out does not exist or this is the en-gb locale
    > ${out}
    echo "# == $dlang" >> ${out}
    echo "# -- $elang" >> ${out}
    sed -e '1,6 d' \
        -e "s/YEAR-MO-DA.*ZONE/${dt}/" \
        -e "s/: [0-9][0-9-]* [0-9][0-9:-]*/: ${dt}/" \
        -e "s/PACKAGE/ballroomdj-4/" \
        -e "s/VERSION/${VERSION}/" \
        -e "s/FULL NAME.*ADDRESS./${xlator}/" \
        -e "s/Bugs-To: /Bugs-To: brad.lanam.di@gmail.com/" \
        -e "s/Language: /Language: ${lang}/" \
        -e "s,Language-Team:.*>,Language-Team: $elang," \
        -e "s/CHARSET/UTF-8/" \
        bdj4.pot >> ${out}
  fi
}

cd po

# If a translation is removed, remember to also remove
# the locale/xx files and directories.

mkpo "$@"

exit 0
