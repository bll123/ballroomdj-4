#!/bin/bash

if [[ $(uname -s) != Darwin ]]; then
  echo "Not running on MacOS"
  exit 1
fi

fn=$1

if [[ ! -f $fn ]]; then
  pwd
  echo "$fn : not found"
  exit 1
fi

llist=$(otool -L $fn | grep icu | sed -e 's,^ *,,' -e 's, (.*,,')
# /opt/local/lib/libicui18n.71.dylib (compatibility version 71.0.0, current version 71.1.0)
# /opt/local/lib/libicui18n.72.dylib -> /opt/local/lib/libicui18n.72.1.dylib
# /opt/local/lib/libicui18n.dylib -> /opt/local/lib/libicui18n.72.dylib
for tlib in $llist; do
  if [[ -f $tlib ]]; then
    # remove all versioning and use the generic link supplied
    # by macports
    link=$(echo $tlib | sed -e 's,\.[0-9.]*\.dylib,.dylib,')
    if [[ -f $link ]]; then
      install_name_tool -change "$tlib" "$link" "$fn"
    fi
  fi
done

exit 0
