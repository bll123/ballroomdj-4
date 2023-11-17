#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
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

if [[ $pkgname == "" || $pkgname = "check" ]]; then
  cd $cwd
  cd check*
  if [[ $? -eq 0 ]]; then
    echo ""
    echo "## build check"
    touch configure
    if [[ $clean == T ]]; then
      make distclean
    fi
    if [[ $conf == T ]]; then
      ./configure --prefix=$INSTLOC --disable-static --disable-subunit
    fi
    make -j $procs
    # makeinfo is not present, so use -k
    make -k install
    if [[ $platform != windows && $clean == T ]]; then
      make distclean
    fi
  fi
fi

