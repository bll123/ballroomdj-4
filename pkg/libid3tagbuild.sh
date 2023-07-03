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
cd libid3tag*
if [ $? -eq 0 ]; then
  echo "## build libid3tag"
  sdir=$(pwd)
  bdir=build

  test -d "${bdir}" && rm -rf "${bdir}"
  cmake -G "$cmg" \
      -B "${bdir}" \
      -DCMAKE_INSTALL_PREFIX="$INSTLOC"
  cmake --build "${bdir}" ${pbld}
  cmake --install "${bdir}"
  rm -rf "${INSTLOC}/lib/cmake"
  rm -rf "${bdir}"
fi