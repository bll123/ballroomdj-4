#!/bin/bash

BASE=bdj4_icon
BASEI=bdj4_icon_inst

for b in $BASE $BASEI; do
  for sz in 256 48 32 16; do
    inkscape $b.svg -w $sz -h $sz -o $b-$sz.png > /dev/null 2>&1
  done
  icotool -c -o $b.ico $b-256.png $b-48.png $b-32.png $b-16.png
  rm -f $b-256.png $b-48.png $b-32.png $b-16.png
done
exit 0
