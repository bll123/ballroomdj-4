#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
ver=1

if [[ $1 == --version ]]; then
  echo ${ver}
  exit 0
fi

if [[ $(uname -s) != Linux ]]; then
  echo "Not running on Linux"
  exit 1
fi

flac12=plocal/lib/libFLAC.so.12
lf=$(find /usr/lib -name 'libFLAC.so.12' -print)
if [[ $lf == "" ]]; then
  lf=$(find /usr/lib -name 'libFLAC.so.8' -print)
  if [[ $lf != "" ]]; then
    # create a symlink for libFLAC
    ln -s ${lf} ${flac12}
  fi
else
  if [[ -h ${flac12} ]]; then
    rm -f ${flac12}
  fi
fi

exit 0
