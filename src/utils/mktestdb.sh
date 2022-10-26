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

TESTMUSIC=test-templates/test-music.txt
TESTDB=test-templates/musicdb.dat
FLAG=data/mktestdb.txt
MUSICFILE=test-music/001-argentinetango.mp3

if [[ ! -d data ]]; then
  echo "No data dir"
  exit 1
fi

if [[ $1 == --force ]]; then
  echo "force db"
  rm -f $FLAG
fi

if [[ ! -f $FLAG ||
    $TESTMUSIC -nt $TESTDB ||
    $TESTMUSIC -nt $FLAG ||
    ! -f $MUSICFILE ]]; then
  rm -f test-music/0[0-9][0-9]-*
  ./bin/bdj4 --tmusicsetup
  touch $FLAG
fi

exit 0
