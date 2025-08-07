#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd img

BASE=bdj4_icon
BASEI=bdj4_icon_inst
BASEC=bdj4_icon_config
BASEMQ=bdj4_icon_marquee
BASEPL=bdj4_icon_player
BASEM=bdj4_icon_manage

for b in ${BASE} ${BASEI}; do
  for sz in 256 48 32 16; do
    inkscape $b.svg -w $sz -h $sz -o $b-$sz.png > /dev/null 2>&1
  done
  icotool -c -o $b.ico $b-256.png $b-48.png $b-32.png $b-16.png
  for sz in 48 32 16; do
    rm -f $b-$sz.png
  done
  mv $b-256.png $b.png
done

for b in ${BASEC} ${BASEMQ} ${BASEPL} ${BASEM}; do
  for sz in 256; do
    inkscape $b.svg -w $sz -h $sz -o $b-$sz.png > /dev/null 2>&1
    mv $b-256.png $b.png
  done
done

cp -pf ${BASE}.ico ../http/favicon.ico

exit 0
