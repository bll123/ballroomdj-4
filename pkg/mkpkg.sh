#!/bin/bash

cwd=$(pwd)
case $cwd in
  */bdj4)
    ;;
  *)
    echo "run from bdj4 top level"
    exit 1
    ;;
esac

systype=$(uname -s)
case $systype in
  Linux)
    tag=linux
    sfx=""
    ;;
  Darwin)
    tag=macos
    sfx=""
    ;;
  MSYS*|MINGW*)
    tag=windows
    sfx=.exe
    ;;
esac

LICDIR=license
rm -f ${LICDIR}/*
cp -f packages/mongoose*/LICENSE ${LICDIR}/mongoose.LICENSE
if [[ $tag == windows ]]; then
  cp -f packages/libressl*/COPYING ${LICDIR}/libressl.LICENSE
fi

(cd src; make distclean > /dev/null 2>&1)
./pkg/mksrcmanifest.sh
(cd src; make; make tclean)
./pkg/mkmanifest.sh

. ./VERSION.txt
BUILD=$(($BUILD+1))
cat > VERSION.txt << _HERE_
VERSION=$VERSION
BUILD=$BUILD
_HERE_

nm=bdj4-${tag}-${VERSION}-installer${sfx}

7z a -sfx ${nm} @install/manifest.txt
case $tag in
  linux)
    7z a bdj4-${VERSION}-src.7z @install/src-manifest.txt
    ;;
esac
