#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

grc=0

systype=$(uname -s)
case $systype in
  Linux)
    os=linux
    platform=unix
    ;;
  Darwin)
    os=macos
    platform=unix
    ;;
  MINGW64*)
    os=win64
    platform=windows
    ;;
  MINGW32*)
    echo "Platform not supported"
    exit 1
    ;;
esac

LOG=src/testall.log
> $LOG

if [[ $os == macos ]]; then
  echo "-- $(date +%T) macports build"
  . ./src/utils/macospath.sh macports
fi

./src/utils/testrun.sh "$@"
grc=$?

TESTBREW=F
if [[ $grc == 0 && TESTBREW == T && $os == macos ]]; then
  echo "-- $(date +%T) homebrew build"
  . ./src/utils/macospath.sh brew
  ./src/utils/testrun.sh "$@"
  grc=$?
fi

exit $grc
