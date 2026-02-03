#!/bin/bash
#
# Copyright 2026 Brad Lanam Pleasant Hill CA
#

# work around a but with cmake 4 on msys2

flist=$(find ./build -name 'build.make')
sed -i 's,-l\([^ \.]*\)\.lib,-l\1,g' ${flist}

exit 0
