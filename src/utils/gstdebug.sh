#!/bin/bash

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

cd tmp/gst

for f in *.dot; do
  nf=$(echo $f | sed -e 's,\.dot$,.svg,' -e 's,^[^-]*-,,')
  dot -Tsvg $f > $nf
done
