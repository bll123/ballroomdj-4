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

tcount=0
pass=0
fail=0

function check {
  tname=$1
  r=$2
  e=$3
  tcount=$(($tcount+1))
  if [[ $r == $e ]]; then
    pass=$(($pass+1))
    trc=0
  else
    echo "    FAIL: $tname"
    echo "     Got: $r"
    echo "Expected: $e"
    fail=$(($fail+1))
    trc=1
  fi
  return $trc
}

cwd=$(pwd)

TARGETDIR=${cwd}/tmp/BDJ4
UNPACKDIR="${cwd}/tmp/bdj4-install"
UNPACKDIRTMP="$UNPACKDIR.tmp"

function resetUnpack {
  test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
  cp -fpr "$UNPACKDIR.tmp" "$UNPACKDIR"
}

test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
./pkg/mkpkg.sh --preskip --insttest
test -d "$UNPACKDIRTMP" && rm -rf "$UNPACKDIRTMP"
cp -fpr "$UNPACKDIR" "$UNPACKDIRTMP"

# main test db : rebuild of standard test database
tname=new-install-no-bdj3
test -d $TARGETDIR && rm -rf $TARGETDIR
./bin/bdj4 --bdj4install --cli --verbose --wait --unattended \
    --targetdir "$TARGETDIR" \
    --unpackdir "$UNPACKDIR"

resetUnpack
tname=re-install-no-bdj3
./bin/bdj4 --bdj4install --cli --verbose --wait --unattended \
    --targetdir "$TARGETDIR" --reinstall \
    --unpackdir "$UNPACKDIR"

resetUnpack
tname=update-no-bdj3
./bin/bdj4 --bdj4install --cli --verbose --wait --unattended \
    --targetdir "$TARGETDIR" \
    --unpackdir "$UNPACKDIR"

test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
test -d "$UNPACKDIRTMP" && rm -rf "$UNPACKDIRTMP"

exit $rc
