#!/bin/bash
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd img

BASE=macos_icon
BASEC=macos_icon_config
BASEI=macos_icon_inst
BASEM=macos_icon_manage
BASEMQ=macos_icon_marquee
BASEP=macos_icon_player
BASEH=macos_icon_help
BASEBPM=macos_icon_bpm

echo -n "Enter host: "
read host
echo -n "Enter port: "
read port

t=tmp.iconset
test -d ${t} || mkdir ${t}

for b in $BASE; do
  for sz in 1024 512 256 128 64 32 16; do
    inkscape $b.svg -w $sz -h $sz -o ${t}/icon_${sz}x${sz}.png > /dev/null 2>&1
  done
  cp ${t}/icon_32x32.png ${t}/icon_16x16@2x.png
  mv ${t}/icon_64x64.png ${t}/icon_32x32@2x.png
  cp ${t}/icon_256x256.png ${t}/icon_128x128@2x.png
  cp ${t}/icon_512x512.png ${t}/icon_256x256@2x.png
  mv ${t}/icon_1024x1024.png ${t}/icon_512x512@2x.png
  rsync -aS --delete ${t} -e "ssh -p ${port}" ${host}:
  ssh -p ${port} ${host} "iconutil --convert icns --output BDJ4.icns ${t}"
  scp -P ${port} ${host}:BDJ4.icns .
  ssh -p ${port} ${host} "rm -rf BDJ4.icns ${t}"
  rm -rf ${t}
done

for b in ${BASE} ${BASEC} ${BASEI} ${BASEM} \
    ${BASEMQ} ${BASESUBT} ${BASEP} ${BASEH} ${BASEBPM}; do
  inkscape $b.svg -w 256 -h 256 -o $b.png > /dev/null 2>&1
done

exit 0
