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

# acrcloud is pre-built
# just install it.

if [[ $pkgname == "" || $pkgname = "acrcloud" ]]; then
  cd $cwd
  if [ $? -eq 0 ]; then
    echo ""
    test -d "$INSTLOC/bin" || mkdir "$INSTLOC/bin"
    fn=acrcloud-${tag}${archtag}${esuffix}
    # do a test for the file, as the source package has the acrcloud
    # binary pre-installed.
    if [[ -f $fn ]]; then
      echo "## install acrcloud"
      cp -f $fn "$INSTLOC/bin/acrcloud${esuffix}"
    fi
  fi
fi

