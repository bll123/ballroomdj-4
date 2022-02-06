#!/bin/bash

# the "ERROR:" output prefixes must exist.

if [[ $# != 1 || ! -d $1 || ! -d $1/data ]]; then
  echo "ERROR: No ballroomdj directory"
  exit 1
fi
dir="$1/data"

systype=$(uname -s | tr '[A-Z]' '[a-z]')
case $systype in
  mingw*|msys*)
    systype=windows
    ;;
esac

bits=64
arch=$(uname -m)
case $arch in
  i?86*)
    bits=32
    ;;
esac

# a) the usual location
# b) for testing: to handle test.dir/data
# c) mac os bdj3
# d) personal install of tcl
# e) personal install of tcl
# f) macports install
# g) brew install (? need to check)
# h) system install   older mac os system install is too old

tclsh=""
if [[ $TCLSH != "" ]]; then
  tclsh=$TCLSH
else
  for f in \
      "$dir/../$systype/64/tcl/bin/tclsh" \
      "$dir/../../$systype/64/tcl/bin/tclsh" \
      "$HOME/Applications/BallroomDJ.app/Contents/MacOS/$systype/64/tcl/bin/tclsh" \
      "$HOME/local/bin/tclsh" \
      "$HOME/bin/tclsh" \
      /opt/local/bin/tclsh \
      /usr/local/bin/tclsh \
      /usr/bin/tclsh \
      ; do
    if [[ -f $f ]]; then
      tclsh=$f
      echo "Located tclsh: $tclsh"
      break
    fi
  done
fi
if [[ $tclsh == "" ]]; then
  echo "ERROR: Unable to locate tclsh"
  exit 1
fi

if [[ ! -d data ]]; then
  echo "ERROR: No bdj4 data directory"
  exit 1
fi

if [[ ! -d conv ]]; then
  echo "ERROR: No bdj4 conv directory"
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
echo "Conversion Complete"
echo "OK"

exit 0
