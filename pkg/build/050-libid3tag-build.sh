#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#


# built on all platforms

if [[ $pkgname == "" || $pkgname = "libid3tag" ]]; then
  cd $cwd
  cd libid3tag*
  if [[ $? -eq 0 ]]; then
    echo ""
    echo "## build libid3tag"
    sdir=$(pwd)
    bdir=build

    if [[ $clean == T ]]; then
      test -d "${bdir}" && rm -rf "${bdir}"
    fi
    if [[ $conf == T ]]; then
      cmake -G "$cmg" \
          -B "${bdir}" \
          -DCMAKE_INSTALL_PREFIX="$INSTLOC" \
          -DCMAKE_C_COMPILER="${CC}"
    fi
    cmake --build "${bdir}" ${cmpbld}
    cmake --install "${bdir}"
    rm -rf "${INSTLOC}/lib/cmake" "${INSTLOC}/lib64/cmake"
    if [[ $platform != windows && $clean == T ]]; then
      rm -rf "${bdir}"
    fi
  fi
fi
