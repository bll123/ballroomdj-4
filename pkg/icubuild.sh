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
    export CC=gcc
    export CXX=g++
    ;;
  Darwin)
    tag=macos
    platform=unix
    export CC=clang
    export CXX=clang++
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
  chmod +x configure install-sh

  make distclean

  tfn=common/unicode/uconfig.h
  if [[ ! -f $tfn.orig ]]; then
    cp -f $tfn $tfn.orig
  fi
  cp -f $tfn.orig $tfn

  # #   define UCONFIG_ONLY_COLLATION 0
  # #   define UCONFIG_NO_LEGACY_CONVERSION 0
  sed -e '/# *define *UCONFIG_ONLY_COLLATION 0/ s,0,1,' \
      -e '/# *define *UCONFIG_NO_LEGACY_CONVERSION 0/ s,0,1,' \
      $tfn > $tfn.n
  mv -f $tfn.n $tfn

  # tools must be enabled to build the data library.
  CFLAGS="-g -O2"
  CXXFLAGS="-g -O2"
  ./configure \
      --prefix=${cwd}/icu \
      --with-data-packaging=library \
      --enable-rpath \
      --disable-extras \
      --disable-icuio \
      --disable-layoutex \
      --disable-tests \
      --disable-samples

  make -j ${procs}
  test -d icu && rm -rf icu
  mkdir icu
  make install
  make distclean
fi
