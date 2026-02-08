#!/bin/bash
#
# Copyright 2026 Brad Lanam Pleasant Hill CA
#

# work around a bug with cmake 4 on msys2

# MSYS Makefiles
flist=$(find ./build -name 'build.make')
sed -i 's,-l\([^ \.]*\)\.lib,-l\1,g' ${flist}
# UNIX Makefiles
# flist=$(find ./build -name 'linkLibs.rsp')
# sed -i 's,-l\([^ \.]*\)\.lib,-l\1,g' ${flist}

exit 0
