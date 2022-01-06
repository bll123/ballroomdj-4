#!/bin/bash

if [[ $# != 1 || ! -d $1  ]]; then
  echo "Usage: $0 <directory>"
  exit 1
fi
dir=$1

if [[ ! -d data ]]; then
  echo "No bdj4 data directory"
  exit 1
fi

if [[ ! -d conv ]]; then
  echo "No bdj4 conv directory"
  exit 1
fi

./conv/configconv.tcl $dir
./conv/danceconv.tcl $dir
./conv/dbconv.tcl $dir
./conv/genreconv.tcl $dir
./conv/levelsconv.tcl $dir
./conv/mlistconv.tcl $dir
./conv/playlistconv.tcl $dir
./conv/ratingconv.tcl $dir
./conv/seqconv.tcl $dir
./conv/sortoptconv.tcl $dir
./conv/autoselconv.tcl $dir
./conv/typeconv.tcl $dir
