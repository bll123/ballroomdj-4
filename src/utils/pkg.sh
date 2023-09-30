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
LINUXMOUNT=/media/sf_${SHNM}
PRIMARYDEV=bll-g7.local

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
    scp -P 166 ${pnm} $PRIMARYDEV:$SHNM/$ISTAGENM
    ;;
esac

exit 0
