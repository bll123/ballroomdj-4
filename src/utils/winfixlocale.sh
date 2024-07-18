#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done

cd locale
for f in *; do
  if [[ -h $f ]]; then
    to=$(ls -ld $f | sed 's,.* ,,')
    rm -f $f
    echo $f $to
    cp -pr $to $f
  fi
done
