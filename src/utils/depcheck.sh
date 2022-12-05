#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

keep=F
if [[ $1 == --keep ]]; then
  keep=T
fi

TIN=dep-in.txt
TSORT=dep-sort.txt
grc=0

# a) check the include file hierarchy for problems.
> $TIN
for fn in */*.c */*.h build/config.h; do
  echo $fn $fn >> $TIN
  grep -E '^#include "' $fn |
      sed -e 's,^#include ",,' \
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

# b) check the .o file hierarchy for problems.
$HOME/bin/lorder $(find ./build -name '*.o') > $TIN
tsort < $TIN > $TSORT
rc=$?
if [[ $rc -ne 0 ]]; then
  grc=$rc
fi

if [[ $keep == F ]]; then
  rm -f $TIN $TSORT > /dev/null 2>&1
fi

exit $grc
