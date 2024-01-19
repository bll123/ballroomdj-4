#!/bin/bash
#
# Copyright 2023-2024 Brad Lanam Pleasant Hill CA
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

# fpcalc is pre-built
# just install it.

if [[ $pkgname == "" || $pkgname = "fpcalc" ]]; then
  cd $cwd
  if [ $? -eq 0 ]; then
    echo ""
    echo "## install ${pkgname}"
    cp -f fpcalc-${tag}${archtag}${esuffix} $INSTLOC/bin/fpcalc${esuffix}
  fi
fi

