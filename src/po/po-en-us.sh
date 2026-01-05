#!/bin/bash
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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

fn=po/en_US.po
echo "-- $(date +%T) creating ${fn}"
awk -f mken_us.awk po/en_GB.po > ${fn}

exit 0
