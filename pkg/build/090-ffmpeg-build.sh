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

# ffmpeg is pre-built
# just install it.

if [[ $pkgname == "" || $pkgname = "ffmpeg" ]]; then
  cd $cwd
  cd ffmpeg*
  if [ $? -eq 0 ]; then
    echo ""
    echo "## install ffmpeg"
    cp -f bin/*.dll $INSTLOC/bin
    cp -f bin/ffmpeg.exe $INSTLOC/bin
    cp -rf include/* $INSTLOC/include
  fi
fi

