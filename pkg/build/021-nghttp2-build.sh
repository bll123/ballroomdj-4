#!/bin/bash
#
# Copyright 2023-2025 Brad Lanam Pleasant Hill CA
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

if [[ $pkgname == "" || $pkgname = "nghttp2" ]]; then
  cd $cwd
  cd nghttp2*
  if [ $? -eq 0 ]; then
    echo ""
    echo "## build nghttp2"
    if [[ $clean == T ]]; then
      make distclean
    fi
    if [[ $conf == T ]]; then
      ./configure $args \
          --prefix=$INSTLOC \
          --enable-lib-only
    fi
    if [[ $clean == T ]]; then
      make clean
    fi
    make -j $procs LIBS="-static-libgcc -static-libstdc++"
    make installdirs
    make install-exec
    make -C lib/includes install-nobase_includeHEADERS
    if [[ $platform != windows && $clean == T ]]; then
      make distclean
    fi
  fi
fi

