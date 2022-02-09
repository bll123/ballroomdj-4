#!/bin/bash
#
# Do not run as root.
#
# This is a binary file.
# To see just the shell script, use the following command:
#     size=2076; head -c $size $0; unset size
#

# size will be one byte larger than the size of this script
size=2076

archivenm=bdj4.tar.gz
unpackdir=bdj4-install

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

test -d $unpackdir && rm -rf $unpackdir
if [[ -d $unpackdir ]]; then
  echo "  Extraction failed (unable to remove $tmpdir/$unpackdir)."
  test -f $archivenm && rm -f $archivenm
  exit 1
fi
tar -x -f $archivenm
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  Extraction failed (tar xf)."
  test -f $archivenm && rm -f $archivenm
  test -d $unpackdir && rm -f $unpackdir
  exit 1
fi

if [[ ! -d $unpackdir ||
    ! -d $unpackdir/install ||
    ! -f $unpackdir/bin/bdj4installer ]]; then
  echo "  Unable to locate installer."
  test -f $archivenm && rm -f $archivenm
  test -d $unpackdir && rm -f $unpackdir
  exit 1
fi

echo "-- Cleaning temporary files."
test -f $archivenm && rm -f $archivenm

cd $unpackdir
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  Extraction failed (cd $unpackdir)."
  test -d $unpackdir && rm -f $unpackdir
  exit 1
fi

if [[ $DISPLAY != "" ]]; then
  echo "-- Starting graphical installer."
  ./bin/bdj4installer $@ > /dev/tty 2>&1
else
  echo "-- Starting console installer."
  ./install/install.sh $@
fi

exit 0
#ENDSCRIPT
