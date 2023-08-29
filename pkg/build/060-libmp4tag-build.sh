#!/bin/bash
#
#

# built on all platforms

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
    make
    make PREFIX=$INSTLOC install
    rm -f ${INSTLOC}/bin/mp4tagcli*
    if [[ $platform != windows && $clean == T ]]; then
      make distclean
    fi
  fi
fi
