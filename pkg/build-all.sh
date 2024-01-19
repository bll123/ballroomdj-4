#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

for bscript in ../pkg/build/*-build.sh; do
  ${bscript}
done

echo "## finalize"
find $INSTLOC -type f -print0 | xargs -0 chmod u+w
find $INSTLOC/bin -type f -print0 | xargs -0 chmod a+rx
exit 0
