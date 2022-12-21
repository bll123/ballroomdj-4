#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
#

archivenm=bdj4-install.tar.gz
unpacktgt=bdj4-install

rundir=$(pwd)
unpackdir=$(pwd)
if [[ -d "$unpackdir/../MacOS" ]]; then
  cd ../..
  unpackdir=$(pwd)
fi
if [[ ! -d "$rundir/install" ]]; then
  echo "  Unable to locate installer."
  test -f $archivenm && rm -f $archivenm
  test -d $unpacktgt && rm -f $unpacktgt
  exit 1
fi

test -f ../$archivenm && rm -f ../$archivenm

cd "$rundir"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  cd $unpacktgt failed."
  test -d $unpacktgt && rm -f $unpacktgt
  exit 1
fi

echo "-- Starting installer."
./bin/bdj4 --bdj4installer ${cli} --unpackdir "$unpackdir" "$@"

exit 0
