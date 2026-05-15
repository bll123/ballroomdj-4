#!/bin/bash
#
# Copyright 2023-2026 Brad Lanam Pleasant Hill CA
#

# built on all platforms

CMAKE=cmake
case ${systype} in
  MINGW64*)
    CMAKE=../cmake-3/bin/cmake
    ;;
esac

if [[ $pkgname == "" || $pkgname = "libmp4tag" ]]; then
  cd $cwd
  cd libmp4tag*
  if [[ $? -eq 0 ]]; then
    echo ""
    echo "## build libmp4tag"
    sdir=$(pwd)
    bdir=build

    if [[ $clean == T ]]; then
      make distclean
    fi
    make CMAKE=${CMAKE} PREFIX=$INSTLOC
    make CMAKE=${CMAKE} install
    rm -f ${INSTLOC}/bin/mp4tagcli*
    if [[ $platform != windows && $clean == T ]]; then
      make distclean
    fi
  fi
fi
