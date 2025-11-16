#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
ver=2

if [[ $1 == --version ]]; then
  echo ${ver}
  exit 0
fi

if [[ $(uname -s) != Linux ]]; then
  echo "Not running on Linux"
  exit 1
fi

pfx=plocal/lib
flacpfx=${pfx}/libFLAC.so.

for fver in 12 14; do
  if [[ -h ${flacpfx}${fver} ]]; then
    rm -f ${flacpfx}${fver}
  fi
done

lf=$(find -P /usr/lib /usr/lib64 /lib64 -name 'libFLAC.so.14' -print 2>/dev/null)
if [[ $lf != "" ]]; then
  exit 0

fi

lf=$(find -P /usr/lib /usr/lib64 /lib64 -name 'libFLAC.so.8' -print)
if [[ $lf != "" ]]; then
  # create a symlink for libFLAC
  rm -f ${flacpfx}12
  rm -f ${flacpfx}14
  ln -s ${lf} ${flacpfx}14
fi
lf=$(find -P /usr/lib /usr/lib64 /lib64 -name 'libFLAC.so.12' -print)
if [[ $lf != "" ]]; then
  # create a symlink for libFLAC
  rm -f ${flacpfx}12
  rm -f ${flacpfx}14
  ln -s ${lf} ${flacpfx}14
fi

exit 0
