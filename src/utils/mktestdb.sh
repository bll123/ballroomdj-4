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

if [[ ! -d data ]]; then
  echo "No data dir"
  exit 1
fi

args=""
infile=$TESTMUSIC
while test $# -gt 0; do
  case $1 in
    --force)
      rm -f $FLAG
      ;;
    --infile)
      args+=$1
      args+=" "
      shift
      infile=$1
      args+=$1
      args+=" "
      ;;
    --outfile)
      args+=$1
      args+=" "
      shift
      outfile=$1
      args+=$1
      args+=" "
      ;;
    *)
      echo "unknown argument $1"
      exit 1
      ;;
  esac
  shift
done

if [[ ! -f $FLAG ||
    $infile != $TESTMUSIC ||
    $outfile != $TESTDB ||
    $infile -nt $TESTDB ||
    $infile -nt $FLAG ]]; then
  rm -f test-music/[0-9][0-9][0-9]-*
  rm -f test-music/[a-z]*
  ./bin/bdj4 --tmusicsetup $args
  touch $FLAG
fi

exit 0
