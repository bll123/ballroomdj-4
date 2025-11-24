#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
ver=2

# This script is not very good.
# There are many variations that it does not cover.

if [[ $1 == --version ]]; then
  echo ${ver}
  exit 0
fi

if [[ $(uname -s) != Linux ]]; then
  echo "Not running on Linux"
  exit 1
fi

pfx=plocal/lib
pfx64=plocal/lib64
flacpfx=${pfx}/libFLAC.so.
flacpfx64=${pfx64}/libFLAC.so.

for fver in 12 14; do
  if [[ -h ${flacpfx}${fver} ]]; then
    rm -f ${flacpfx}${fver} ${flacpfx64}${fver}
  fi
done

lf=$(find -P /usr/lib /usr/lib64 /lib64 -name 'libFLAC.so.14' -print 2>/dev/null)
if [[ $lf != "" ]]; then
  exit 0
fi

lf=$(find -P /usr/lib /usr/lib64 /lib64 -name 'libFLAC.so.12' -print)

if [[ $lf == "" ]]; then
  lf=$(find -P /usr/lib /usr/lib64 /lib64 -name 'libFLAC.so.8' -print)
fi

if [[ $lf != "" ]]; then
  case ${lf} in
    */lib64/*)
      fpfx=${flacpfx64}
      ;;
    *)
      fpfx=${flacpfx}
      ;;
  esac
  # create a symlink for libFLAC
  ln -s ${lf} ${fpfx}14
fi

exit 0
