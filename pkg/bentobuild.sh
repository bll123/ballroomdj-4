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
cd Bento4*
if [ $? -eq 0 ]; then
  echo "## build Bento4"
  sdir=$(pwd)
  bdir="${sdir}/cmakebuild"

  fn=CMakeLists.txt
  if [[ ! -f ${fn}.orig ]]; then
    cp -pf ${fn} ${fn}.orig
  fi
  cp -pf ${fn}.orig ${fn}
  # add_library(ap4 STATIC ${AP4_SOURCES})
  sed '/^add_library.ap4/ s,STATIC,SHARED,' ${fn} > ${fn}.tmp
  mv -f ${fn}.tmp ${fn}

  test -d "${bdir}" && rm -rf "${bdir}"
  cmake -G "$cmg" \
      -S "${sdir}" \
      -B "${bdir}" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$INSTLOC" \
      -DBUILD_APPS=OFF
  cmake --build "${bdir}" ${pbld}
  cmake --install "${bdir}"
  rm -rf "${INSTLOC}/lib/cmake"
fi
