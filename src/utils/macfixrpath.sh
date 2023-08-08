#!/bin/bash

if [[ $# -ne 1 ]]; then
  echo "$0: bad target"
  exit 1
fi

target=$1
#LPATH=@executable_path
#LPATH=@loader_path
# the rpath has been updated within the cmake files to point
# to the correct directories.
LPATH=@rpath

# /Volumes/Users/bll/bdj4/packages/icu/lib/libicudata.72.dylib
# /Volumes/Users/bll/bdj4/bin/../plocal/lib/libvorbisfile.3.dylib
# @rpath/libid3tag.0.16.3
count=0
cmd="install_name_tool "
for l in libicudata libicui18n libicuuc \
    libvorbisfile.3 libvorbis.0; do
  path=$(otool -L $target | grep $l | sed 's,^[^/]*,,;s,dylib .*,dylib,')
  if [[ $path != "" ]]; then
    echo "== library updated : $target" >&2
    fulllib=$(echo $path | sed 's,.*/,,')
    cmd+="-change $path ${LPATH}/${fulllib} "
    count=$(($count+1))
  fi
done

if [[ $count -gt 0 ]]; then
  cmd+=" $target"
  eval $cmd
fi

exit 0
