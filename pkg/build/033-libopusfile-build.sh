#!/bin/bash
#
# Copyright 2023-2024 Brad Lanam Pleasant Hill CA
#

# dependents:
#   libogg
#   libopus

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

if [[ $pkgname == "" || $pkgname = "libopusfile" ]]; then
  cd $cwd
  cd opusfile*
  if [[ $? -eq 0 ]]; then
    echo ""
    echo "## build libopusfile"
    if [[ $clean == T ]]; then
      make distclean
    fi
    if [[ $conf == T ]]; then
      ./configure $args \
          --prefix=$INSTLOC \
          --disable-static \
          --disable-http \
          --disable-examples
    fi
    if [[ $clean == T ]]; then
      make clean
    fi
    make -j $procs LIBS="-static-libgcc -static-libstdc++"
    make install
    if [[ $platform != windows && $clean == T ]]; then
      make distclean
    fi
  fi
fi

