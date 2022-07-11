#!/bin/bash

TIN=si-in.txt
TSORT=si-sort.txt
grc=0

# a) check the include file hierarchy for problems.
> $TIN
for fn in */*.c */*.h build/config.h; do
  echo $fn $fn >> $TIN
  sed -n -e '/^#include "/p' $fn |
      sed -e 's,^#include ",,' -e 's, *//.*,,' \
      -e 's,"$,,' -e "s,^,$fn ," >> $TIN
done
tsort < $TIN > $TSORT
rc=$?
if [[ $rc -ne 0 ]]; then
  grc=$rc
fi

rm -f $TIN $TSORT > /dev/null 2>&1

# b) check the .o file hierarchy for problems.
$HOME/bin/lorder $(find ./build -name '*.o') > $TIN
tsort < $TIN > $TSORT
rc=$?
if [[ $rc -ne 0 ]]; then
  grc=$rc
fi

rm -f $TIN $TSORT > /dev/null 2>&1

exit $rc
