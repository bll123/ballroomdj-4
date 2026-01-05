#!/bin/bash
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#

# 'ERROR' and 'OK' are used by the gui to determine when the script
# is finished.

if [[ $# != 2 || ! -d $1 || ! -d "$1/data" || ! -d $2 ]]; then
  echo "ERROR: Invalid arguments"
  exit 1
fi
bdj3dir="$1/data"
datatopdir="$2"

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
      "$bdj3dir/$systype/64/tcl/bin/tclsh" \
      "$bdj3dir/../$systype/64/tcl/bin/tclsh" \
      "$bdj3dir/../../$systype/64/tcl/bin/tclsh" \
      "$HOME/Applications/BallroomDJ.app/Contents/MacOS/$systype/64/tcl/bin/tclsh" \
      "$HOME/local/bin/tclsh" \
      "$HOME/bin/tclsh" \
      /opt/local/bin/tclsh \
      /usr/local/bin/tclsh \
      /usr/bin/tclsh \
      ; do
    if [[ -f $f ]]; then
      tclsh=$f
      echo "   Located tclsh: $tclsh"
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

for fn in conv/0*.tcl; do
  "$tclsh" $fn "$bdj3dir" "$datatopdir"
done

echo "OK"

exit 0
