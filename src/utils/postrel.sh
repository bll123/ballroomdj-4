#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

INSTSTAGE=$HOME/vbox_shared/bdj4inst

count=$(ls -1 $INSTSTAGE/bdj4-installer-* | wc -l)
if [[ $count -ne 8 ]]; then
  echo "Failed: not all platforms built."
  exit 1
fi

mv -f $INSTSTAGE/bdj4-installer-* Archive

rm -rf $INSTSTAGE
mkdir $INSTSTAGE

exit 0
