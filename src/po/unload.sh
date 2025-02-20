#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
KEEP=F
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
    --keep)
      KEEP=T
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

for f in po/*.po; do
  base=$(echo $f | sed -e 's,^po/,,' -e 's,\.po$,,' -e 's,^web-,,')
  tbase=$(echo $base | sed -e 's,_,-,')
  if [[ $tbase == nb-NO ]]; then
    tbase=no-NO
  fi
  if [[ ! -d $TMP/$tbase ]]; then
    tbase=$(echo $tbase | sed 's,\(..\).*,\1,')
  fi
  if [[ $tbase == nb ]]; then
    tbase=no
  fi
  if [[ ! -d $TMP/$tbase ]]; then
    continue
  fi

  echo "$f found"
  cp -f $f $f.backup
  mv -f $f $f.bak
  echo "Processing $f"
  sed -n '1,2 p' $f.bak > $f
  sed -n '3,$ p' $TMP/$tbase/en_GB.po >> $f
done

for f in web/*.po; do
  base=$(echo $f | sed -e 's,^web/,,' -e 's,\.po$,,' -e 's,^web-,,')
  tbase=$(echo $base | sed -e 's,_,-,')
  if [[ $tbase == nb-NO ]]; then
    tbase=no-NO
  fi
  if [[ ! -d $TMP/$tbase ]]; then
    tbase=$(echo $tbase | sed 's,\(..\).*,\1,')
  fi
  if [[ $tbase == nb ]]; then
    tbase=no
  fi
  if [[ ! -d $TMP/$tbase ]]; then
    continue
  fi

  echo "$f found"
  cp -f $f $f.backup
  mv -f $f $f.bak
  echo "Processing $f"
  sed -n '1,2 p' $f.bak > $f
  sed -n '3,$ p' $TMP/$tbase/web-en_GB.po >> $f
done

test -d $TMP && rm -rf $TMP
if [[ $KEEP == F ]]; then
  rm -f 'BallroomDJ 4 '*.zip > /dev/null 2>&1
fi

exit 0
