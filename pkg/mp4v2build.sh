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

cd $cwd
cd mp4v2*
if [ $? -eq 0 ]; then
  echo "## build mp4v2"
  sdir=$(pwd)
  bdir="${sdir}/build"

  test -d "${bdir}" && rm -rf "${bdir}"
  cmake -G "$cmg" \
      -S "${sdir}" \
      -B "${bdir}" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$INSTLOC" \
      -DBUILD_UTILS=off
  cmake --build "${bdir}" ${pbld} --verbose

  cmake --install "${bdir}"
fi

