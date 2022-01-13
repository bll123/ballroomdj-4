#!/bin/bash

if [[ $# != 1 || ! -d $1 || ! -d $1/../data ]]; then
  echo "No ballroomdj directory"
  exit 1
fi
dir=$1

systype=$(uname -s | tr '[A-Z]' '[a-z]')

bits=64
arch=$(uname -m)
case $arch in
  i?86*)
    bits=32
    ;;
esac

tclsh=""
if [[ $TCLSH != "" ]]; then
  tclsh=$TCLSH
else
  for f in "$dir/../../$systype/64/tcl/bin/tclsh" \
      /usr/bin/tclsh $HOME/local/bin/tclsh $HOME/bin/tclsh \
      /opt/local/bin/tclsh; do
    if [[ -f $f ]]; then
      tclsh=$f
      break
    fi
  done
fi
if [[ $tclsh == "" ]]; then
  echo "ERR: Unable to locate tclsh"
  exit 1
fi

if [[ ! -d data ]]; then
  echo "No bdj4 data directory"
  exit 1
fi

if [[ ! -d conv ]]; then
  echo "No bdj4 conv directory"
  exit 1
fi

"$tclsh" ./conv/configconv.tcl $dir
"$tclsh" ./conv/danceconv.tcl $dir
"$tclsh" ./conv/dbconv.tcl $dir
"$tclsh" ./conv/genreconv.tcl $dir
"$tclsh" ./conv/levelsconv.tcl $dir
"$tclsh" ./conv/mlistconv.tcl $dir
"$tclsh" ./conv/playlistconv.tcl $dir
"$tclsh" ./conv/ratingconv.tcl $dir
"$tclsh" ./conv/seqconv.tcl $dir
"$tclsh" ./conv/sortoptconv.tcl $dir
"$tclsh" ./conv/autoselconv.tcl $dir
"$tclsh" ./conv/typeconv.tcl $dir
"$tclsh" ./conv/statusconv.tcl $dir
