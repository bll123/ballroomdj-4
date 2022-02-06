#!/bin/bash

TIN=si-in.txt
TSORT=si-sort.txt

> $TIN
for fn in *.c *.h build/config.h; do
  echo $fn $fn >> $TIN
  sed -n -e '/^#include "/p' $fn |
      sed -e 's,^#include ",,' -e 's,"$,,' -e "s,^,$fn ," >> $TIN
done
tsort < $TIN | egrep -v '(config.h|check.h)' > $TSORT
rc=$?

rm -f $TIN $TSORT > /dev/null 2>&1
exit $rc
