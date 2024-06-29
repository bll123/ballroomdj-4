#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src
cwd=$(pwd)

rm -f po/*~ po/po/*~ po/web/*~ > /dev/null 2>&1

. ../VERSION.txt
export VERSION
dt=$(date '+%F')
export dt

cd po

fn=po/en_GB.po
./po-mk.sh ${fn} en_GB english/gb 'English (GB)' 'Automatically generated'
# The Queue_noun tag must be replaced with the correct noun.
sed -i -e '/"Queue_noun"/ { n ; s,"","Queue", ; }' ${fn}

exit 0
