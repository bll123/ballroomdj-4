#!/bin/bash
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#

#
# This script will contain major build differences that require
# a clean and re-build.
# - Different package managers
# - Different UI flavors
#
# Testing with configuration differences should be in testrun.sh
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
  MINGW64*|CYGWIN*)
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

TDEFAULT=F
TPKGSRC=F
THOMEBREW=F
for arg in "$@"; do
  case $arg in
    --testmacports|--testdefault)
      TDEFAULT=T
      ;;
    --testpkgsrc)
      TPKGSRC=T
      ;;
    --testhomebrew)
      THOMEBREW=T
      ;;
  esac
done

if [[ $TDEFAULT == F && $TPKGSRC == F && $THOMEBREW == F ]]; then
  TDEFAULT=T
  TPKGSRC=F       # off unless requested
  THOMEBREW=T
fi

if [[ $grc == 0 && \
    $TPKGSRC == T && \
    $os == macos ]]; then
  echo "-- $(date +%T) MacOS pkgsrc build"
  echo "-- $(date +%T) MacOS pkgsrc build" >> $LOG
  . ./src/utils/macospath.sh pkgsrc
  ./src/utils/testrun.sh "$@"
  grc=$?
fi

# homebrew will never work on intel
if [[ $grc == 0 && \
    $THOMEBREW == T && \
    ${archtag} == applesilicon && \
    $os == macos ]]; then
  echo "-- $(date +%T) MacOS homebrew build"
  echo "-- $(date +%T) MacOS homebrew build" >> $LOG
  . ./src/utils/macospath.sh homebrew
  ./src/utils/testrun.sh "$@"
  grc=$?
fi

if [[ $os == macos ]]; then
  echo "-- $(date +%T) MacOS macports build"
  echo "-- $(date +%T) MacOS macports build" >> $LOG
  . ./src/utils/macospath.sh macports
fi

if [[ $grc == 0 && \
    $TDEFAULT == T ]]; then
  ./src/utils/testrun.sh "$@"
  grc=$?
fi

exit $grc
