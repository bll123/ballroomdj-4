#!/bin/bash

TIN=si-in.txt
TSORT=si-sort.txt

> $TIN
for fn in *.c *.h; do
  echo $fn $fn >> $TIN
  sed -n -e '/^#include "/p' $fn |
      sed -e 's,^#include ",,' -e 's,"$,,' -e "s,$, $fn," >> $TIN
  case $fn in
    *.c)
      sed -n -e '/^#include "/p' $fn |
          sed -e 's,^#include ",,' -e 's,"$,,' \
          -e 's,\.h$,.c,' -e "s,$, $fn," >> $TIN
      ;;
  esac
done
tsort < $TIN | egrep -v '(config.h|check.h)' > $TSORT
rc=$?
exit $?

rm -f $TIN $TSORT > /dev/null 2>&1
exit 0
