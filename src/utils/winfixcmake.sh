#!/bin/bash
#
# Copyright 2026 Brad Lanam Pleasant Hill CA
#

flist=$(find ./build -name 'build.make')
sed -i 's,-l\([^ \.]*\)\.lib,-l\1,g' ${flist}

exit 0
