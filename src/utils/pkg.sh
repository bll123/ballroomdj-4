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
LINUXMOUNT=/media/sf_${SHNM}
PRIMARYDEV=bll-g7.local
LOG=pkg.log

. ./src/utils/pkgnm.sh
pkgnmgetdata
pnm=$(pkginstnm)

if [[ $pn_devtag != "" ]]; then
  echo "development tag is on"
  exit 1
fi

echo "-- $(date +%T) building"
(
  cd src
  make distclean
)

(
  cd src
  case ${pn_dist} in
    -opensuse15)
      # change this in utils/testall.sh also
      make CC=gcc-13 CXX=g++-13
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
      case ${pn_dist} in
        -opensuse15)
          cp -pf ${pnm} $LINUXMOUNT/$ISTAGENM
          ;;
        *)
          scp -P 166 ${pnm} $PRIMARYDEV:$SHNM/$ISTAGENM
          ;;
      esac
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
