#!/bin/bash

manifest=$1
checksumfn=$2

systype=$(uname -s)

shaprog=sha512sum
win=F
macos=F
linux=F
case ${systype} in
  Linux)
    nproc=$(getconf _NPROCESSORS_ONLN)
    linux=T
    ;;
  Darwin)
    nproc=$(getconf _NPROCESSORS_ONLN)
    shaprog="shasum -a 512"
    macos=T
    ;;
  MINGW64*|MINGW32*)
    nproc=$NUMBER_OF_PROCESSORS
    win=T
    ;;
esac

inpfx=ckin
outpfx=ckout.

rm -f tmp/${inpfx}??
rm -f tmp/${outpfx}*

# building the checksum file on windows is slow.
# this isn't necessary on linux/macos.
if [[ $win == T ]]; then
  split -n l/$nproc ${manifest} tmp/${inpfx}
else
  cp -f ${manifest} tmp/${inpfx}aa
fi

cwd=$(pwd)
(
cd $(dirname $(dirname ${manifest}))
if [[ $macos == T ]]; then
  cd ../..
fi
count=0
for cfn in ${cwd}/tmp/${inpfx}??; do
  count=$((count+1))
  for fn in $(cat ${cfn}); do
    if [[ -f $fn ]]; then
      ${shaprog} -b $fn >> ${cwd}/tmp/${outpfx}${count} &
    fi
  done
done
)

cat tmp/${outpfx}* > ${checksumfn}
rm -f tmp/${inpfx}??
rm -f tmp/${outpfx}*

exit 0
