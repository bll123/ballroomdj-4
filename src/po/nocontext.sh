#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

ctx=F
while read line ; do
  case $line in
    "#."*)
      ctx=T
      ;;
    "msgid"*)
      if [[ $ctx == F ]]; then
        echo $line
      fi
      ctx=F
      ;;
  esac
done < src/po/en_GB.po

exit 0
