#!/bin/bash
#
# Copyright 2023-2024 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

SFUSER=bll123
PROJECT=ballroomdj4

cd voices

function getlastdate {
  td=$1

  lfile=""
  for f in $td/*; do
    if [[ $lfile == "" || $f -nt $lfile ]]; then
      lfile=$f
    fi
  done
  tdt=$(date -ud "@$(stat --format '%Y' $f)" '+%Y%m%d')
  echo $tdt
}

for d in $(find . -name '[a-z]*' -type d -print); do
  nd=$(echo $d | sed 's,^\./,,')
  dt=$(getlastdate $nd)
  test -f $nd.zip && rm -f $nd.zip
  zip -q -r $nd.zip $nd
  mv $nd.zip $nd-$dt.zip
done

rsync -v -e ssh *.zip \
  ${SFUSER}@frs.sourceforge.net:/home/frs/project/${PROJECT}/announcements/

exit 0
