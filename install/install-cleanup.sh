#!/bin/bash

LOG=""

if [[ ! -d install ]]; then
  echo "Unable to locate install directory."
  exit 1
fi

for f in $(cat install/cleanuplist.txt); do
  test -f $f && rm -f $f
  test -d $f && rm -rf $f
done

exit 0
