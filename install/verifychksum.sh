#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#

systype=$(uname -s)

shaprog="sha512sum --status"
macosbase=""
case ${systype} in
  Darwin)
    shaprog="shasum -a 512 -s"
    # on mac os, the current location is above Contents/MacOS,
    # and this script is started from there.
    macosbase=Contents/MacOS/
    ;;
esac

checksumfn=${1:-${macosbase}install/checksum.txt}

${shaprog} -c ${checksumfn}
rc=$?

if [[ $rc -eq 0 ]]; then
  echo OK
else
  echo NG
fi

exit 0
