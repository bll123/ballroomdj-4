#!/bin/bash

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

./src/utils/mktestsetup.sh --force

# creates about 32k files
for a in a b c d e f g h i; do
  mkdir $a
  for b in a b c d e f g h i j k l m n o p q r s t u v w x y z; do
    mkdir $a/$b
    cp -p *.mp3 *.ogg *.opus *.m4a *.flac *.ogx $a/$b
  done
  echo "fin: $a"
done
