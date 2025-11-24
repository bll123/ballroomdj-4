#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

#
# This script will contain major build differences.
# - Different package managers
# - Different UI flavors
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

systype=$(uname -s)
arch=$(uname -m)

case $systype in
  Linux)
    os=linux
    platform=unix
    ;;
  Darwin)
    os=macos
    if [[ $arch == x86_64 ]]; then
      archtag=intel
    fi
    if [[ $arch == arm64 ]]; then
      archtag=applesilicon
    fi
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
grc=0

TESTPKGSRC=T
if [[ $grc == 0 && \
    TESTPKGSRC == T && \
    $os == macos ]]; then
  echo "-- $(date +%T) pkgsrc build"
  . ./src/utils/macospath.sh pkgsrc
  ./src/utils/testrun.sh "$@"
  grc=$?
fi

# homebrew will never work on intel
TESTHOMEBREW=T
if [[ $grc == 0 && \
    TESTHOMEBREW == T && \
    ${archtag} == applesilicon && \
    $os == macos ]]; then
  echo "-- $(date +%T) homebrew build"
  . ./src/utils/macospath.sh homebrew
  ./src/utils/testrun.sh "$@"
  grc=$?
fi

if [[ $os == macos ]]; then
  echo "-- $(date +%T) macports build"
  . ./src/utils/macospath.sh macports
fi

if [[ $grc == 0 ]]; then
  ./src/utils/testrun.sh "$@"
  grc=$?
fi

exit $grc
