#!/bin/bash

checksumfn=${1:-install/checksum.txt}

systype=$(uname -s)

shaprog="sha512sum --status"
case ${systype} in
  Darwin)
    shaprog="shasum -a 512 -s"
    ;;
esac

${shaprog} -c ${checksumfn}
rc=$?

if [[ $rc -eq 0 ]]; then
  echo OK
else
  echo NG
fi

exit 0
