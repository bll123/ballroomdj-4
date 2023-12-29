#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

TESTMUSIC=test-templates/test-music.txt
TESTDB=test-templates/musicdb.dat
FLAG=data/mktestdb.txt

if [[ ! -d data ]]; then
  echo "No data dir"
  exit 1
fi

args=""
infile=$TESTMUSIC
rmflag=F
outfile=""
keepmusic=F
MUSICDIR=test-music
while test $# -gt 0; do
  case $1 in
    --debug)
      args+=$1
      args+=" "
      shift
      args+=$1
      args+=" "
      ;;
    --force)
      rm -f $FLAG
      ;;
    --emptydb)
      args+=$1
      args+=" "
      rmflag=T
      ;;
    --infile)
      args+=$1
      args+=" "
      shift
      infile=$1
      args+=$1
      args+=" "
      rmflag=T
      ;;
    --outfile)
      args+=$1
      args+=" "
      shift
      outfile=$1
      args+=$1
      args+=" "
      rmflag=T
      ;;
    --keepdb)
      ;;
    --keepmusic)
      args+=$1
      args+=" "
      keepmusic=T
      ;;
    --seconddir)
      args+=$1
      args+=" "
      shift
      # altdir can be a full path with spaces and intl characters
      args+="'$1'"
      args+=" "
      ;;
    --atibdj4)
      # ignored
      ;;
    --plimpv)
      # ignored
      ;;
    --nodbcopy)
      # ignored
      ;;
    --dbupmusicdir)
      args+=$1
      args+=" "
      shift
      args+="'$1'"
      args+=" "
      MUSICDIR=$1
      ;;
    *)
      echo "unknown argument $1" >&2
      exit 1
      ;;
  esac
  shift
done

if [[ ! -f $FLAG ||
    $infile != $TESTMUSIC ||
    ($outfile != "" && $outfile != $TESTDB) ||
    $infile -nt $TESTDB ||
    $infile -nt $FLAG ]]; then
  if [[ ! -d data || ! -d src || ! -d tmp ]]; then
    echo "invalid current dir $(pwd)" >&2
    exit 1
  fi
  if [[ $keepmusic == F && $MUSICDIR != "" && -d $MUSICDIR ]]; then
    rm -rf $MUSICDIR/*
  fi
  # need to use eval to get the arguments quoted properly
  eval ./bin/bdj4 --tmusicsetup $args
  touch $FLAG
fi

if [[ $rmflag == T ]]; then
  rm -f $FLAG
fi

echo $outfile
exit 0
