#!/bin/bash
#
# Do not run as root.
#
# This is a binary file.
# To see just the shell script, use the following command:
#     size=1708; head -c $size $0; unset size
#

# size will be one byte larger than the size of this script
size=1708

archivenm=bdj4.tar.gz
installdir=bdj4-install

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

echo "BallroomDJ 4 Installation Startup"
script="$0"

echo "  Extracting archive to $tmpdir."
test -f "$tmpdir/$archivenm" && rm -f "$tmpdir/$archivenm"

nsize=$((size+1))

tail -c +${nsize} "$script" > "$tmpdir/$archivenm"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  Extraction via 'tail' failed."
  exit 1
fi

cd $tmpdir
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  Extraction failed (cd $tmpdir)."
  exit 1
fi

test -d $installdir && rm -rf $installdir
if [[ -d $installdir ]]; then
  echo "  Extraction failed (unable to remove $tmpdir/$installdir)."
  exit 1
fi
tar -x -f $archivenm
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  Extraction failed (tar xf)."
  exit 1
fi

if [[ ! -d $installdir ||
    ! -d $installdir/install ||
    ! -f $installdir/bin/bdj4installer ]]; then
  echo "  Unable to locate installer."
  exit 1
fi

echo "  Cleaning temporary files."
test -f $arhivenm && rm -f $archivenm

cd $installdir
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "  Extraction failed (cd $installdir)."
  exit 1
fi

if [[ $DISPLAY != "" ]]; then
  echo "  Starting graphical installer."
  $installdir/bin/bdj4installer
else
  echo "  Starting console installer."
  $installdir/install/install.sh
fi

exit 0
#ENDSCRIPT
