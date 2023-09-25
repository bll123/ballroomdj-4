#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

./pkg/mkpkg.sh
. ./src/utils/pkgnm.sh
nm=$(pkginstnm)
systype=$(uname -s)
case $systype in
  Linux)
    isprimary=F
    if [[ -f devel/primary.txt ]]; then
      isprimary=T
    fi
    if [[ $isprimary == T ]]; then
      cp -pf ${nm} $HOME/vbox_shared
    else
      cp -pf ${nm} /media/sf_vbox_shared
    fi
    ;;
  Windows)
    cp -pf ${nm} /z
    ;;
  Darwin)
    # this would be better off as a pull from the main build machine
    scp -P 166 ${nm} 192.168.2.49:vbox_shared
    ;;
esac
