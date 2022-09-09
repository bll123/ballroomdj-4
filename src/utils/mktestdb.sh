#!/bin/bash

cwd=$(pwd)
case ${cwd} in
  */utils)
    cd ../..
    ;;
  */src)
    cd ..
    ;;
esac

if [[ ! -d data ]]; then
  exit 1
fi

if [[ $1 == --force ]]; then
  rm -f data/mktestdb.txt
fi

if [[ test-templates/test-music.txt -nt test-templates/musicdb.dat ||
    test-templates/test-music.txt -nt data/mktestdb.txt ||
    ! -f test-music/001-argentinetango.mp3 ]]; then
  rm -f test-music/0[0-9][0-9]*
  ./bin/bdj4 --tmusicsetup
  touch data/mktestdb.txt
fi

exit 0
