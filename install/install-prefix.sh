#!/bin/bash
#
# Do not run as root.
#
# This is a binary file.
# To see just the shell script, use the following command:
#     size=2227; head -c $size $0; unset size
#

# size will be one byte larger than the size of this script
size=2227

archivenm=bdj4.tar.gz
unpacktgt=bdj4-install

tmpdir=${TMPDIR:-}
if [[ $tmpdir == "" || ! -d $tmpdir ]]; then
  tmpdir=${TEMP:-}
fi
if [[ $tmpdir == "" || ! -d $tmpdir ]]; then
  tmpdir=${TMP:-}
fi
if [[ $tmpdir == "" || ! -d $tmpdir ]]; then
  tmpdir=/tmp
fi

echo "-- BallroomDJ 4 Installation Startup"
script="$0"

echo "-- Extracting archive to $tmpdir."
test -f "$tmpdir/$archivenm" && rm -f "$tmpdir/$archivenm"

nsize=$((size+1))

tail -c +${nsize} "$script" > "$tmpdir/$archivenm"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  Extraction via 'tail' failed."
  test -f "$tmpdir/$archivenm" && rm -f "$tmpdir/$archivenm"
  exit 1
fi

cd $tmpdir
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  Extraction failed (cd $tmpdir)."
  test -f "$tmpdir/$archivenm" && rm -f "$tmpdir/$archivenm"
  exit 1
fi

test -d $unpacktgt && rm -rf $unpacktgt
if [[ -d $unpacktgt ]]; then
  echo "  Extraction failed (unable to remove $tmpdir/$unpacktgt)."
  test -f $archivenm && rm -f $archivenm
  exit 1
fi
tar -x -f $archivenm
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  Extraction failed (tar xf)."
  test -f $archivenm && rm -f $archivenm
  test -d $unpacktgt && rm -f $unpacktgt
  exit 1
fi

rundir=$unpacktgt
if [[ -d "$unpacktgt" ]]; then
  if [[ -d "$unpacktgt/Contents" ]]; then
    rundir="$unpacktgt/Contents/MacOS"
  fi
  if [[ ! -d "$rundir/install" ]]; then
    echo "  Unable to locate installer."
    test -f $archivenm && rm -f $archivenm
    test -d $unpacktgt && rm -f $unpacktgt
    exit 1
  fi
fi

echo "-- Cleaning temporary files."
test -f $archivenm && rm -f $archivenm

cd $rundir
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  Extraction failed (cd $unpacktgt)."
  test -d $unpacktgt && rm -f $unpacktgt
  exit 1
fi

if [[ $DISPLAY != "" ]]; then
  echo "-- Starting graphical installer."
  ./bin/bdj4 --installer --unpackdir "$tmpdir/$unpacktgt" $@ > /dev/tty 2>&1
else
  echo "-- Starting console installer."
  ./install/install.sh --unpackdir "$tmpdir/$unpacktgt" $@
fi

exit 0
#ENDSCRIPT
