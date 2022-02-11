#!/bin/bash

BASE=bdj4_icon
BASEI=bdj4_icon_inst

for b in $BASE $BASEI; do
  convert -resize 64x64 $b.png $b-64.png
  convert -resize 48x48 $b.png $b-48.png
  convert -resize 32x32 $b.png $b-32.png
  convert -resize 16x16 $b.png $b-16.png
  icotool -c -o $b.ico $b.png $b-64.png $b-48.png $b-32.png $b-16.png
  if [[ $b == $BASE ]]; then
    png2icns BDJ4.icns $b.png $b-48.png $b-32.png $b-16.png \
#      > /dev/null
  fi
  rm -f $b-64.png $b-48.png $b-32.png $b-16.png
done
exit 0
