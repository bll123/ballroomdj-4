#!/bin/bash

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd packages
cwd=$(pwd)

systype=$(uname -s)
arch=$(uname -m)
case $systype in
  Linux)
    tag=linux
    platform=unix
    archtag=
    ;;
  Darwin)
    tag=macos
    platform=unix
    case $arch in
      x86_64)
        archtag=-intel
        ;;
      arm64)
        archtag=-applesilicon
        ;;
    esac
    ;;
esac

procs=$(getconf _NPROCESSORS_ONLN)
echo "= procs: $procs"

cd $cwd
cd icu-release*
if [ $? -eq 0 ]; then
  echo "## build icu"

  cd icu4c/source

  make distclean

#      --disable-renaming
  ./configure \
      --prefix=${cwd}/icu \
      --with-data-packaging=library \
      --enable-rpath \
      --disable-extras \
      --disable-icuio \
      --disable-layoutex \
      --disable-tools \
      --disable-tests \
      --disable-samples

  make -j ${procs}
  make install
  make distclean
fi
