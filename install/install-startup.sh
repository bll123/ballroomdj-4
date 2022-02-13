#!/bin/bash
#

archivenm=bdj4-install.tar.gz
unpacktgt=bdj4-install

echo "-- BDJ4 Installation Startup"

unpackdir=$(pwd)
rundir=$(pwd)
if [[ -d "$rundir/Contents" ]]; then
  rundir="$rundir/Contents/MacOS"
fi
if [[ ! -d "$rundir/install" ]]; then
  echo "  Unable to locate installer."
  test -f $archivenm && rm -f $archivenm
  test -d $unpacktgt && rm -f $unpacktgt
  exit 1
fi

echo "-- Cleaning temporary files."
test -f ../$archivenm && rm -f ../$archivenm

cd "$rundir"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  cd $unpacktgt failed."
  test -d $unpacktgt && rm -f $unpacktgt
  exit 1
fi

reinstall=""
if [[ $1 == "--reinstall" ]]; then
  reinstall=--reinstall
fi
if [[ $2 == "--reinstall" ]]; then
  reinstall=--reinstall
fi

if [[ $1 == "--cli" ]]; then
  echo "-- Starting console installer."
  ./bin/bdj4 --installer --guidisabled --unpackdir "$unpackdir" $reinstall
else
  echo "-- Starting graphical installer."
  ./bin/bdj4 --installer --unpackdir "$unpackdir" $reinstall > /dev/tty 2>&1
fi

exit 0
