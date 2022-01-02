#!/bin/bash

TMPF=si-in.txt
TSORT=si-sort.txt

> $TMPF
for fn in *.c *.h; do
  echo $fn $fn >> $TMPF
  sed -n -e '/^#include "/p' $fn |
      sed -e 's,^#include ",,' -e 's,"$,,' -e "s,$, $fn," >> $TMPF
  case $fn in
    *.c)
      sed -n -e '/^#include "/p' $fn |
          sed -e 's,^#include ",,' -e 's,"$,,' \
          -e 's,\.h$,.c,' -e "s,$, $fn," >> $TMPF
      ;;
  esac
done
tsort < $TMPF | egrep -v '(config.h|check.h)' > $TSORT

