#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src
cwd=$(pwd)

keep=F
if [[ $1 == --keep ]]; then
  keep=T
fi

systype=$(uname -s)

INCTC=dep-inctest.c
INCTO=dep-inctest.o
INCTOUT=dep-inctest.log
TIIN=dep-inc-in.txt
TISORT=dep-inc-sort.txt
TOIN=dep-obj-in.txt
TOSORT=dep-obj-sort.txt
TUFUNC=dep-ufunc.txt
TUFUNCOUT=dep-unused.txt
DEPCOMMON=dep-libcommon.txt
DEPBASIC=dep-libbasic.txt
DEPBDJ4=dep-libbdj4.txt
grc=0

# check for unused functions
echo "## checking for unused functions"
grep -E '^[a-zA-Z0-9]* \(' */*.c */*.cpp */*.m |
  grep -E -v '(:main|:fprintf|uinull|uimacos|KEEP|UNUSED|TESTING)' |
  grep -E -v '(rlogError|rlogProcBegin|rlogProcEnd)' |
  sed -e 's, (.*,,' -e 's,.*:,,' |
  sort > ${TUFUNC}
> ${TUFUNCOUT}
for pat in $(cat ${TUFUNC}); do
  #  <sp>func<sp>(
  #  ->func<sp>(
  #  <sp>func,
  #  (func,
  #  (func)
  #  "func"
  # do not search the check/*/*.c files
  grep -E "([ >\(]${pat} \(|[ \(]${pat}[,)]|\"${pat}\")" \
      */*.c */*.cpp */*.m > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo $pat >> ${TUFUNCOUT}
    echo "unused function $pat"
  fi
done

# check for missing copyrights
echo "## checking for missing copyright"

# this is run from the src/ directory
for fn in */*.c */*/*.c */*.cpp */*.m */*.h */ui/*.h \
    */*.sh ../*/*.sh ../pkg/*/*.sh ../pkg/windows/version.rc.in \
    CMakeLists.txt */CMakeLists.txt Makefile ../README.md \
    po/Makefile* */*.awk config.h.in */*.cmake ../pkg/macos/*.plist; do
  case $fn in
    *src/tt.sh|*src/z.sh)
      continue
      ;;
    build/*)
      continue
      ;;
    *mongoose.c)
      continue
      ;;
    *mpris-root.[ch]|*mpris-player.[ch])
      # generated file
      continue
      ;;
    *potemplates.c)
      # temporary file
      continue
      ;;
    ../dev/*)
      continue
      ;;
    utils/dumpvars.cmake)
      continue
      ;;
    utils/*.sh)
      # most of this can be skipped
      continue
      ;;
  esac
  grep "Copyright" $fn > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "$fn : missing copyright"
    grc=$rc
  fi
done
if [[ $grc != 0 ]]; then
  exit $grc
fi

# check to make sure the include files can be compiled w/o dependencies
echo "## checking include file compilation"
test -f $INCTOUT && rm -f $INCTOUT
for fn in include/*.h include/ui/*.h; do
  case $fn in
    *uimacos-int.h)
      if [[ $systype == Linux ]]; then
        continue
      fi
      ;;
  esac
  bfn=$(echo $fn | sed 's,include/,,')
  cat > $INCTC << _HERE_
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "${bfn}"

int
main (int argc, char *argv [])
{
  return 0;
}
_HERE_
  cc -c \
      -DBDJ4_UI_GTK3=1 \
      -I build -I include \
      $(pkg-config --cflags gtk+-3.0) \
      $(pkg-config --cflags glib-2.0) \
      -I mongoose \
      $INCTC >> $INCTOUT 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "compile of $bfn failed"
    if [[ $rc -ne 0 ]]; then
      grc=$rc
    fi
  fi
  rm -f $INCTC $INCTO
done
rm -f $INCTC $INCTO
if [[ $grc -ne 0 ]]; then
  exit $grc
fi
rm -f $INCTOUT

# check the include file hierarchy for problems.
echo "## checking include file hierarchy"
> $TIIN
for fn in */*.c */*/*.c */*.cpp */*.m */*.h */ui/*.h build/config.h; do
  echo $fn $fn >> $TIIN
  grep -E '^# *include "' $fn |
      sed -e 's,^# *include ",,' \
      -e 's,".*$,,' \
      -e "s,^,$fn include/," >> $TIIN
done
tsort < $TIIN > $TISORT
rc=$?

if [[ $keep == F ]]; then
  rm -f $TIIN $TISORT > /dev/null 2>&1
fi
if [[ $rc -ne 0 ]]; then
  grc=$rc
  exit $grc
fi

# check the object file hierarchy for problems.
echo "## checking object file hierarchy"
#
./utils/lorder $(find ./build -name '*.o') > $TOIN
tsort < $TOIN > $TOSORT
rc=$?
if [[ $rc -ne 0 ]]; then
  grc=$rc
fi

if [[ $keep == T ]]; then
  grep -E '/(libbdj4common|objosutils|objlauncher|objosenv|objdirutil|objfileop|objosprocess|objstring|objtmutil).dir/' $TOSORT |
      sed -e 's,.*/,,' > $DEPCOMMON
  grep -E '/(libbdj4basic).dir/' $TOSORT |
      sed -e 's,.*/,,' > $DEPBASIC
  grep -E '/(libbdj4).dir/' $TOSORT |
      sed -e 's,.*/,,' > $DEPBDJ4
fi
if [[ $keep == F ]]; then
  rm -f $TIIN $TISORT $TOIN $TOSORT $TUFUNC $TUFUNCOUT \
      $DEPCOMMON $DEPBASIC $DEPBDJ4 > /dev/null 2>&1
fi
rm -f $INCCT $INCTO $INCTOUT

exit $grc
