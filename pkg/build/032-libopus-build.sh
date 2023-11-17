#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
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

if [[ $pkgname == "" || $pkgname = "libopus" ]]; then
  cd $cwd
  cd opus-1*
  if [[ $? -eq 0 ]]; then
    echo ""
    echo "## build libopus"
    if [[ $clean == T ]]; then
      make distclean
    fi
    if [[ $conf == T ]]; then
      ./configure $args \
          --prefix=$INSTLOC \
          --disable-static
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

