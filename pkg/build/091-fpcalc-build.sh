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
    fn=fpcalc-${tag}${archtag}${esuffix}
    if [[ -f $fn ]]; then
      echo "## install fpcalc"
      cp -f $fn $INSTLOC/bin/fpcalc${esuffix}
    fi
  fi
fi

