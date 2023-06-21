#!/bin/bash
#
#

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
    cmg="Unix Makefiles"
    pbld="--parallel"
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
    cmg="Unix Makefiles"
    pbld="--parallel"
    ;;
  MINGW64*)
    tag=win64
    platform=windows
    archtag=
    export CC=gcc
    export CXX=g++
    cmg="MSYS Makefiles"
    pbld=""
    export LIBS="-static-libgcc -static-libstdc++"
    ;;
  MINGW32*)
    echo "Platform not supported"
    exit 1
    ;;
esac

INSTLOC=$cwd/../plocal
test -d $INSTLOC || mkdir -p $INSTLOC

procs=$(getconf _NPROCESSORS_ONLN)
echo "= procs: $procs"
if [[ $platform == windows ]]; then
  procs=1
fi

cd $cwd
cd libvorbis*
if [ $? -eq 0 ]; then
  echo "## build libvorbis"
  sdir=$(pwd)
  bdir=build

  make distclean
  ./configure \
      CFLAGS=-g \
      --prefix=$INSTLOC \
      --disable-static \
      --disable-examples
  make clean
  make -j $procs LIBS="-static-libgcc -static-libstdc++"
  make install
  make distclean
fi
