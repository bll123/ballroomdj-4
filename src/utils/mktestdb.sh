#!/bin/bash

cwd=$(pwd)
while : ; do
  case ${cwd} in
    */bdj4)
      break
      ;;
    *)
      cd ..
      cwd=$(pwd)
      ;;
  esac
done

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
    --emptydb)
      args+=$1
      args+=" "
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
  case $(pwd) in
    */bdj4)
      ;;
    *)
      echo "invalid current dir $(pwd)"
      exit 1
      ;;
  esac
  if [[ -d test-music ]]; then
    rm -rf test-music/*
  fi
  ./bin/bdj4 --tmusicsetup $args
  touch $FLAG
fi

exit 0
