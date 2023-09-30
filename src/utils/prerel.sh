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

isprimary=F
if [[ -f devel/primary.txt ]]; then
  isprimary=T
fi
if [[ $isprimary == F ]]; then
  echo "Failed: not primary development platform"
  exit 1
fi

. ./src/utils/pkgnm.sh

pnm=$(pkginstnm)
case ${pnm} in
  *-dev)
    echo "Failed: marked as development package."
    exit 1
    ;;
esac

rm -rf $INSTSTAGE
mkdir $INSTSTAGE

rm -f bdj4-installer-*-dev

exit 0
