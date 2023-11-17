#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

# the gettext supplied with msys2 cannot handle windows account names
# with international characters.

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

if [[ $pkgname == "" || $pkgname = "gettext" ]]; then
  cd $cwd
  cd gettext*
  if [[ $? -eq 0 ]]; then
    echo ""
    echo "## build gettext"
    if [[ $clean == T ]]; then
      make distclean
    fi
    if [[ $conf == T ]]; then
      ./configure --prefix=$INSTLOC
    fi
    make -j $procs
    make install
    if [[ $platform != windows && $clean == T ]]; then
      make distclean
    fi
  fi
fi

