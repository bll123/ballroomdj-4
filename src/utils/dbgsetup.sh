#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

BASEDIR=/home/bll/s/bdj4

if [[ ! -d data ]]; then
  echo "No data directory"
  exit 1
fi
cwd=$(pwd)

systype=$(uname -s)
voli=libvolpa
case $systype in
  Linux)
    os=linux
    platform=unix
    voli=libvolpa
    ;;
  Darwin)
    os=macos
    platform=unix
    voli=libvolmac
    ;;
  MINGW64*)
    os=win64
    platform=windows
    voli=libvolwin
    ;;
  MINGW32*)
    echo "Platform not supported"
    exit 1
    ;;
esac

hostname=$(hostname)

(
  cd data
  for fn in *; do
    if [[ ! -d $fn ]]; then continue; fi
    if [[ $fn == ${hostname} ]]; then continue; fi
    if [[ $fn == profile00 ]]; then continue; fi
    ohostname=$fn
    break
  done

  if [[ ! -d ${hostname} ]]; then
    cp -rp ${ohostname} ${hostname}
  fi
)

if [[ ! -d img/profile00 ]]; then
  test -d img || mkdir -p img
  cp -rp ${BASEDIR}/templates/img img/profile00
fi

tfn=data/${hostname}/bdjconfig.txt
sed -e "/^VOLUME/ { n ; s/.*/..${voli}/ ; }" \
    -e "/^DIRMUSIC/ { n ; s,.*,..${BASEDIR}/test-music, ; }" \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

exit 0
