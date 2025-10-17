#!/bin/bash
#
# Copyright 2023-2025 Brad Lanam Pleasant Hill CA
#

case ${systype} in
  Linux)
    ;;
  Darwin)
    ;;
  MINGW64*)
    ;;
esac

# acr fp extractor is pre-built
# just install it.

if [[ $pkgname == "" || $pkgname = "acr" ]]; then
  cd $cwd
  if [ $? -eq 0 ]; then
    echo ""
    test -d "$INSTLOC/bin" || mkdir "$INSTLOC/bin"
    fn=acr_extr-${tag}${esuffix}
    # do a test for the file, as the source package has the acrcloud
    # binary pre-installed.
    if [[ -f $fn ]]; then
      echo "## install acr"
      cp -f $fn "$INSTLOC/bin/acr_extr${esuffix}"
    fi
  fi
fi

