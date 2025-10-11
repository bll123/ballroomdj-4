#!/bin/bash
#
# Copyright 2023-2025 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

SHNM=vbox_shared
ISTAGENM=bdj4inst
INSTSTAGE=$HOME/$SHNM/$ISTAGENM

count=$(ls -1 $INSTSTAGE/bdj4-installer-* | wc -l)
# 2023-12-4 fedora testing failed due to weird volume issue
# 2024-1-15 fedora dropped
# 2024-1-15 manjaro linux (arch) dropped (icu updated)
# 2024-8-11 debian 11 dropped
# 2025-8-9  debian 13 added
# 2025-10-11 opensuse 16 added
if [[ $count -ne 7 ]]; then
  echo "Failed: not all platforms built."
  exit 1
fi

mv -f $INSTSTAGE/bdj4-installer-* Archive

rm -rf $INSTSTAGE
mkdir $INSTSTAGE

rm -f bdj4-src-*-dev.*
mv -f bdj4-src-* Archive

exit 0
