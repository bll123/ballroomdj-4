#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

cd check
for f in *.c */*.c; do
  ca=$(egrep START_TEST $f| wc -l)
  cb=$(egrep END_TEST $f | wc -l)
  if [[ $ca -ne $cb ]]; then
    echo $f
  fi
done
exit 0

