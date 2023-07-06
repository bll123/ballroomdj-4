#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src/po
cwd=$(pwd)

TMP=xlatetmp

test -d $TMP && rm -rf $TMP
mkdir $TMP

UNPACKONLY=F
zipfile="$(echo 'BallroomDJ 4'*.zip)"
while test $# -gt 0; do
  case $1 in
    --zipfile)
        shift
        zipfile=$1
        shift
      ;;
    --unpack)
        UNPACKONLY=T
        shift
    ;;
  esac
done

if [[ ! -f "$zipfile" ]]; then
  echo "$zipfile not found"
  exit 1
fi

(
  cd $TMP
  echo "Unpacking $zipfile"
  unzip -q "../$zipfile"
)

if [[ $UNPACKONLY == T ]];then
  exit 0
fi

for f in *.po; do
  base=$(echo $f | sed 's,\.po$,,')
  if [[ ! -d $TMP/$base ]]; then
    base=$(echo $base | sed 's,\(..\).*,\1,')
  fi
  if [[ ! -d $TMP/$base ]]; then
    continue
  fi

  echo "$f found"
  cp -f $f $f.backup
  mv -f $f $f.bak
  echo "Processing $f"
  sed -n '1,2 p' $f.bak > $f
  sed -n '3,$ p' $TMP/$base/en_GB.po >> $f
done

test -d $TMP && rm -rf $TMP
rm -f 'BallroomDJ 4 '*.zip > /dev/null 2>&1

exit 0
