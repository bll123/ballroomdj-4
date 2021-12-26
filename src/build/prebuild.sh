#!/bin/bash

stype=$(uname -s)

case $stype in
  Darwin)
    ;;
  Linux)
    ;;
  *BSD)
    ;;
  MINGW*|MSYS*|CYGWIN*)
    ICON=bdj4_icon.ico
    cp -f ../i/$ICON .
    echo "id ICON $ICON" > launcher.rc
    windres launcher.rc -O coff -o launcher.res
    rm -f $ICON launcher.rc
    RESOURCEFILES=launcher.res
    export RESOURCEFILES
    ;;
esac
