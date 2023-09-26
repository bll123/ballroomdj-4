#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

ISTAGENM=bdj4inst
INSTSTAGE=$HOME/vbox_shared/$ISTAGENM
LINUXMOUNT=/media/sf_vbox_shared

./pkg/mkpkg.sh
. ./src/utils/pkgnm.sh
pnm=$(pkginstnm)
systype=$(uname -s)
case $systype in
  Linux)
    isprimary=F
    if [[ -f devel/primary.txt ]]; then
      isprimary=T
    fi
    if [[ $isprimary == T ]]; then
      cp -pf ${pnm} $INSTSTAGE
    else
      cp -pf ${pnm} $LINUXMOUNT/$ISTAGENM
    fi
    ;;
  MINGW*)
    cp -pf ${pnm} /z/$ISTAGENM
    ;;
  Darwin)
    ;;
esac

exit 0
