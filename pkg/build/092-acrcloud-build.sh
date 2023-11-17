#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

case ${systype} in
  Linux)
    ;;
  Darwin)
    ;;
  MINGW64*)
    ;;
esac

# fpcalc is pre-built
# just install it.

if [[ $pkgname == "" || $pkgname = "acrcloud" ]]; then
  cd $cwd
  if [ $? -eq 0 ]; then
    echo ""
    echo "## install ${pkgname}"
    test -d "$INSTLOC/bin" || mkdir "$INSTLOC/bin"
    cp -f acrcloud-${tag}${archtag}${esuffix} "$INSTLOC/bin/acrcloud${esuffix}"
  fi
fi

