#!/bin/bash

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

SFUSER=bll123
PROJECT=ballroomdj4

cd voices

for d in $(find . -name '[a-z]*' -type d -print); do
  nd=$(echo $d | sed 's,^\./,,')
  zip -q -r $d.zip $d
  dt=$(date '+%Y%m%d')
  mv $d.zip $d-$dt.zip
done

rsync -v -e ssh *.zip \
  ${SFUSER}@frs.sourceforge.net:/home/frs/project/${PROJECT}/announcements/

exit 0
