#!/bin/bash

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
    echo "## install fpcalc"
    cp -f fpcalc-win64.exe $INSTLOC/bin/fpcalc.exe
  fi
fi

