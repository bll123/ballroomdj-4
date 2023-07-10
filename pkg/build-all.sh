#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

# exported variables
# clean   - T/F
# conf    - T/F
# pkgname - package name to build
# INSTLOC - installation location
# cwd     - current working dir (packages)

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

INSTLOC=${cwd}/plocal
test -d $INSTLOC || mkdir -p $INSTLOC
export INSTLOC

. ./pkg/build/build-setup.sh

clean=T
conf=T
pkgname=""

while test $# -gt 0;do
  case $1 in
    --noclean)
      clean=F
      ;;
    --noconf)
      conf=F
      ;;
    --pkg|--pkgname)
      shift
      pkgname=$1
      ;;
    *)
      ;;
  esac
  shift
done
export clean conf pkgname

# if an older compiler is needed...
# if [ -d /winlibs/mingw${b}/bin ]; then
#   PATH=/winlibs/mingw${b}/bin:$PATH
# fi

cd packages
cwd=$(pwd)
export cwd

for bs in ../pkg/build/*-build.sh; do
  ${bs}
done

echo "## finalize"
find $INSTLOC -type f -print0 | xargs -0 chmod u+w
exit 0
