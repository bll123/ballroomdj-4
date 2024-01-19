#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

INCTC=inctest.c
INCTO=inctest.o
INCTOUT=inctest.log
TIN=dep-in.txt
TSORT=dep-sort.txt
DEPCOMMON=dep-libcommon.txt
DEPBASIC=dep-libbasic.txt
DEPBDJ4=dep-libbdj4.txt
grc=0

# a) check to make sure the include files can be compiled w/o dependencies
echo "## checking include file compilation"
test -f $INCTOUT && rm -f $INCTOUT
for fn in include/*.h include/ui/*.h; do
  bfn=$(echo $fn | sed 's,include/,,')
  cat > $INCTC << _HERE_
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "${bfn}"

int
main (int argc, char *argv [])
{
  return 0;
}
_HERE_
  cc -c \
      -DBDJ4_USE_GTK3=1 \
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

# b) check the include file hierarchy for problems.
echo "## checking include file hierarchy"
> $TIN
for fn in */*.c */*/*.c */*.cpp */*.m */*.h */ui/*.h build/config.h; do
  echo $fn $fn >> $TIN
  grep -E '^# *include "' $fn |
      sed -e 's,^# *include ",,' \
      -e 's,".*$,,' \
      -e "s,^,$fn include/," >> $TIN
done
tsort < $TIN > $TSORT
rc=$?

if [[ $keep == F ]]; then
  rm -f $TIN $TSORT > /dev/null 2>&1
fi
if [[ $rc -ne 0 ]]; then
  grc=$rc
  exit $grc
fi

# c) check the object file hierarchy for problems.
echo "## checking object file hierarchy"
#
./utils/lorder $(find ./build -name '*.o') > $TIN
tsort < $TIN > $TSORT
rc=$?
if [[ $rc -ne 0 ]]; then
  grc=$rc
fi

if [[ $keep == T ]]; then
  grep -E '/(libbdj4common|objosutils|objlauncher|objosenv|objdirutil|objfileop|objosprocess|objstring|objtmutil).dir/' $TSORT |
      sed -e 's,.*/,,' > $DEPCOMMON
  grep -E '/(libbdj4basic).dir/' $TSORT |
      sed -e 's,.*/,,' > $DEPBASIC
  grep -E '/(libbdj4).dir/' $TSORT |
      sed -e 's,.*/,,' > $DEPBDJ4
fi
if [[ $keep == F ]]; then
  rm -f $TIN $TSORT $DEPCOMMON $DEPBASIC $DEPBDJ4 > /dev/null 2>&1
fi
rm -f $INCCT $INCTO $INCTOUT

exit $grc
