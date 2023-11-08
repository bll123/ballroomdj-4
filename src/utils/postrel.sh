#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

SHNM=vbox_shared
ISTAGENM=bdj4inst
INSTSTAGE=$HOME/$SHNM/$ISTAGENM

count=$(ls -1 $INSTSTAGE/bdj4-installer-* | wc -l)
if [[ $count -ne 8 ]]; then
  echo "Failed: not all platforms built."
  exit 1
fi

mv -f $INSTSTAGE/bdj4-installer-* Archive

rm -rf $INSTSTAGE
mkdir $INSTSTAGE

rm -f bdj4-src-*-dev.*
mv -f bdj4-src-* Archive

exit 0
