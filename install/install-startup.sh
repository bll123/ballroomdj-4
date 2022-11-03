#!/bin/bash
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

reinstall=""
cli=""
while test $# -gt 0; do
  if [[ $1 == "--reinstall" ]]; then
    reinstall=--reinstall
  fi
  if [[ $1 == "--cli" ]]; then
    cli=--cli
  fi
  shift
done

echo "-- Starting installer."
./bin/bdj4 --bdj4installer ${cli} --unpackdir "$unpackdir" $reinstall

exit 0
