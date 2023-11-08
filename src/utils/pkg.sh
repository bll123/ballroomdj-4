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
LOG=pkg.log

. ./src/utils/pkgnm.sh
pkgnmgetdata
pnm=$(pkginstnm)

echo "-- $(date +%T) building"
(
  cd src
  make distclean
)

(
  cd src
  case ${pn_dist} in
    -opensuse)
      make CC=gcc-12 CXX=g++-12
      ;;
    *)
      make
      ;;
  esac
) > $LOG 2>&1

./pkg/mkpkg.sh

if [[ ! -f ${pnm} ]]; then
  echo "-- Package was not created."
  exit 1
fi

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

rm -f $LOG

exit 0
