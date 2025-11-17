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

# the pre-built binary gtk packages from smartos.org is not built correctly
TESTPKGSRC=F
if [[ $grc == 0 && TESTPKGSRC == T && $os == macos ]]; then
  echo "-- $(date +%T) pkgsrc build"
  . ./src/utils/macospath.sh pkgsrc
  ../pkg/build-all.sh --pkg libid3tag >> $LOG 2>&1
  ./src/utils/testrun.sh "$@"
  grc=$?
  # revert back to the default macports
  . ./src/utils/macospath.sh macports
  ../pkg/build-all.sh --pkg libid3tag >> $LOG 2>&1
fi

# homebrew will never work on intel
# homebrew is not working on apple silicon, and I'm not inclined to debug it.
TESTHOMEBREW=F
if [[ $grc == 0 && TESTHOMEBREW == T && $os == macos ]]; then
  echo "-- $(date +%T) homebrew build"
  . ./src/utils/macospath.sh homebrew
  ../pkg/build-all.sh --pkg libid3tag >> $LOG 2>&1
  ./src/utils/testrun.sh "$@"
  grc=$?
  # revert back to the default macports
  . ./src/utils/macospath.sh macports
  ../pkg/build-all.sh --pkg libid3tag >> $LOG 2>&1
fi

exit $grc
