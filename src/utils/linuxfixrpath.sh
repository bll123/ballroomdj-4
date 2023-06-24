#!/bin/bash

if [[ $# -ne 1 ]]; then
  echo "$0: bad target"
  exit 1
fi

target=$1

patchelf \
  --set-rpath \$ORIGIN:\$ORIGIN/../plocal/lib \
  "${target}"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "== $target : failed to update paths" >&2
fi

exit 0
