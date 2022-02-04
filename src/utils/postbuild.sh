#!/bin/bash

stype=$(uname -s)

case $stype in
  Darwin)
    install_name_tool -change '../bin/libbdj4.dylib' \
        '@executable_path/libbdj4.dylib' \
        $@
    install_name_tool -change '../bin/libbdj4common.dylib' \
        '@executable_path/libbdj4common.dylib' \
        $@
    install_name_tool -change '../bin/libbdj4player.dylib' \
        '@executable_path/libbdj4player.dylib' \
        $@
    install_name_tool -change '../bin/libbdj4ui.dylib' \
        '@executable_path/libbdj4ui.dylib' \
        $@
    # libvlc is handled with DYLD_FALLBACK_PATH
    ;;
  Linux)
    patchelf --set-rpath '$ORIGIN/../bin' $@
    ;;
  *BSD)
    ;;
  MINGW*|MSYS*|CYGWIN*)
    ICON=bdj3_icon.ico
    rm -f $ICON launcher.rc launcher.res
    ;;
esac
