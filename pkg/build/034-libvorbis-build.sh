#!/bin/bash
#
#

# dependents:
#   libogg

# built on all platforms

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
