#!/bin/bash

TMP=xlatetmp

test -d $TMP && rm -rf $TMP
mkdir $TMP

(
  cd $TMP
  unzip -q ../"BallroomDJ 4 "*.zip
)

if [[ $1 == -unpack ]];then
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
  mv -f $f $f.bak
  echo "Unpacking $f"
  sed -n '1,2 p' $f.bak > $f
  sed -n '3,$ p' $TMP/$base/en_GB.po >> $f
done

test -d $TMP && rm -rf $TMP
rm -f "BallroomDJ 4 "*.zip

exit 0
