#!/bin/bash
#
# Copyright 2022-2025 Brad Lanam Pleasant Hill CA
#

if [[ $# -ne 1 ]]; then
  echo "$0: bad target"
  exit 1
fi

target=$1

if [[ ! -f $target ]]; then
  echo "$0: $target does not exist"
  exit 1
fi

#LPATH=@executable_path
#LPATH=@loader_path
# the rpath has now been updated by cmake files to point
# to the correct paths (using @loader_path).
LPATH=@rpath

starget=$(basename $target)
liblist=""

# /Volumes/Users/bll/bdj4/packages/icu/lib/libicudata.72.dylib
count=0
cmd="install_name_tool "
for l in libicudata libicui18n libicuuc; do
  path=$(otool -L $target | grep $l | sed 's,^[^/]*,,;s,dylib .*,dylib,')
  if [[ $path != "" ]]; then
    liblist+=$l
    liblist+=" "
    fulllib=$(echo $path | sed 's,.*/,,')
    cmd+="-change $path ${LPATH}/${fulllib} "
    count=$(($count+1))
  fi
done

if [[ $count -gt 0 ]]; then
  echo "== updated : $starget ($liblist)" >&2
  cmd+=" $target"
  eval $cmd
fi

exit 0
