#!/bin/bash

stype=$(uname -s)

case $stype in
  Darwin)
    install_name_tool -change '../bin/libbdj.dylib' \
        '@executable_path/../bin/libbdj.dylib' \
        $@
    ;;
  Linux)
    patchelf --set-rpath '$ORIGIN/../bin' $@
    ;;
  *BSD)
    ;;
  MSYS*|CYGWIN*)
    ;;
esac
