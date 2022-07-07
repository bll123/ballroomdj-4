#!/bin/bash

manifest=$1
checksumfn=$2

systype=$(uname -s)

shaprog=sha512sum
win=F
case ${systype} in
  Linux)
    nproc=$(getconf _NPROCESSORS_ONLN)
    ;;
  Darwin)
    nproc=$(getconf _NPROCESSORS_ONLN)
    shaprog="shasum -a 512"
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
# this really isn't necessary on linux/macos.
split -n l/$nproc ${manifest} tmp/${inpfx}

(
cd $(dirname $(dirname ${manifest}))
count=0
for cfn in ../${inpfx}??; do
  count=$((count+1))
  for fn in $(cat ${cfn}); do
    if [[ -f $fn ]]; then
      ${shaprog} -b $fn >> ../${outpfx}${count} &
    fi
  done
done
)

cat tmp/${outpfx}* > ${checksumfn}
rm -f tmp/${inpfx}??
rm -f tmp/${outpfx}*

if [[ $win == T ]]; then
  # for windows, the '*' binary indicator before the filename needs
  # to be removed so that the checksum verification batch file is
  # much simpler.
  ed ${checksumfn} << _HERE_ > /dev/null
1,$ s, \*, ,
w
q
_HERE_
fi

exit 0
