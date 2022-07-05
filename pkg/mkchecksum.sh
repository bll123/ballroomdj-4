#!/bin/bash

manifest=$1
checksumfn=$2

systype=$(uname -s)

shaprog=sha512sum
win=F
case ${systype} in
  Darwin)
    shaprog="shasum -a 512"
    ;;
  MINGW64*|MINGW32*)
    win=T
    ;;
esac

(
  for fn in $(cat ${manifest}); do
    if [[ -f $fn ]]; then
      ${shaprog} -b $fn
    fi
  done
) > ${checksumfn}

if [[ $win == T ]]; then
  # for windows, the '*' binary indicator before the filename needs
  # to be removed so that the checksum verification batch file is
  # much simpler.
  ed ${checksumfn} << _HERE_
1,$ s, \*, ,
w
q
_HERE_
fi

exit 0
