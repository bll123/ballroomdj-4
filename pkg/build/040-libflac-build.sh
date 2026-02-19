#!/bin/bash
#
# Copyright 2023-2026 Brad Lanam Pleasant Hill CA
#

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

if [[ $pkgname == "" || $pkgname = "libflac" ]]; then
  cd $cwd
  cd flac*
  if [[ $? -eq 0 ]]; then
    echo ""
    echo "## build libflac"
    if [[ $clean == T ]]; then
      make distclean
    fi
    if [[ $conf == T ]]; then
      ./configure $args \
          --prefix=$INSTLOC \
          --disable-static \
          --disable-programs \
          --disable-examples
    fi
    if [[ $clean == T ]]; then
      make clean
    fi
    make -j $procs # LIBS="-static-libgcc -static-libstdc++"
    make install
    # don't need the c++ version
    rm -f $INSTLOC/bin/libFLAC++* $INSTLOC/lib/libFLAC++* $INSTLOC/lib/pkgconfig/flac++.pc
    rm -rf $INSTLOC/include/FLAC++
    if [[ $platform != windows && $clean == T ]]; then
      make distclean
    fi
  fi
fi

