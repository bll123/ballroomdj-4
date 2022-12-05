#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

BASE=bdj4_icon
BASEI=bdj4_icon_inst

echo -n "Enter host: "
read host
echo -n "Enter port: "
read port

t=tmp.iconset
mkdir ${t}

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
exit 0
