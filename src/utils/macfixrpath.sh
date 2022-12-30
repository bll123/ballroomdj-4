#!/bin/bash

if [[ $# -ne 1 ]]; then
  echo "$0: bad target"
  exit 1
fi

target=$1

install_name_tool \
  -change "@rpath/libbdj4.dylib" \
    "@executable_path/libbdj4.dylib" \
  -change "@rpath/libbdj4common.dylib" \
    "@executable_path/libbdj4common.dylib" \
  -change "@rpath/libbdj4basic.dylib" \
    "@executable_path/libbdj4basic.dylib" \
  -change "@rpath/libbdj4pli.dylib" \
    "@executable_path/libbdj4pli.dylib" \
  -change "@rpath/libbdj4ui.dylib" \
    "@executable_path/libbdj4ui.dylib" \
  -change "@rpath/libbdj4vol.dylib" \
    "@executable_path/libbdj4vol.dylib" \
  -change "@rpath/libuigtk3.dylib" \
    "@executable_path/libuigtk3.dylib" \
  $target

# /Volumes/Users/bll/bdj4/packages/icu/lib/libicudata.72.dylib
count=0
cmd="install_name_tool "
for l in libicudata libicui18n libicuuc; do
  path=$(otool -L $target | grep $l | sed 's,^[^/]*,,;s,dylib .*,dylib,')
  if [[ $path != "" ]]; then
    fulllib=$(echo $path | sed 's,.*/,,')
    cmd+="-change $path @executable_path/../plocal/lib/${fulllib} "
    count=$(($count+1))
  fi
done

if [[ $count -gt 0 ]]; then
  cmd+=" $target"
  eval $cmd
fi

exit 0
