#!/bin/bash
#
# Copyright 2023-2026 Brad Lanam Pleasant Hill CA
#

# dependents:
#   libogg

case ${systype} in
  Linux)
    exit 0
    ;;
  Darwin)
    exit 0
    ;;
  MINGW64*)
    ;;
esac

if [[ $pkgname == "" || $pkgname = "libvorbis" ]]; then
  cd $cwd
  cd libvorbis*
  if [ $? -eq 0 ]; then
    echo ""
    echo "## build libvorbis"
    sdir=$(pwd)
    bdir=build

    if [[ $clean == T ]]; then
      make distclean
    fi
    if [[ $conf == T ]]; then
      ./configure \
          CFLAGS=-g \
          --prefix=$INSTLOC \
          --disable-static \
          --disable-examples
    fi
    if [[ $clean == T ]]; then
      make clean
    fi
    make -j $procs
    make install
    if [[ $platform != windows && $clean == T ]]; then
      make distclean
    fi
  fi
fi
