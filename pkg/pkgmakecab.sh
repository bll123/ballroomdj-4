#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

cwd=$(cygpath -d $(pwd) | sed 's,\\,/,g')
FDIRS=find-dirs.txt
FALL=find-all.txt
FLIST=makecab.ddf
MCDIR=makecab.dir
MCOUT=bdj4-install.cab
MCLOG=makecab.log

echo "   $(date +%T) creating directory listing"
> $FALL
find bdj4-install -type d -print > $FDIRS
echo "   $(date +%T) creating file list"
for d in $(cat $FDIRS); do
  echo "=== $d" >> $FALL
  find $d -mindepth 1 -maxdepth 1 -type f -print >> $FALL
done

echo "   $(date +%T) creating .ddf file"
awk -v CWD=$cwd '
BEGIN { gsub (/\//, "\\", CWD); }

{
  gsub (/\//, "\\", $0);
  if (/^=== /) {
    sub (/^=== /, "");
    print ".Set DestinationDir=" $0;
  } else {
    print "\"" CWD "\\" $0 "\" /inf=no";
  }
}' $FALL \
> $FLIST

echo "   $(date +%T) creating cabinet"
cat > $MCDIR << _HERE_
.New Cabinet
.Set DiskDirectory1=.
.Set CabinetNameTemplate=$MCOUT
.set GenerateInf=OFF
.Set Cabinet=ON
.Set Compress=ON
.Set CompressionType=LZX
.Set CompressionMemory=21
.Set UniqueFiles=ON

.Set CabinetFileCountThreshold=0
.Set FolderFileCountThreshold=0
.Set FolderSizeThreshold=0
.Set MaxCabinetSize=0
.Set MaxDiskFileCount=0
.Set MaxDiskSize=0

.set RptFileName=nul
.set InfFileName=nul

.set MaxErrors=1
_HERE_

unix2dos -q $MCDIR
unix2dos -q $FLIST

# avoid strange 'bad address' errors from executing cmd.exe
# from within msys2.
CEXE=$(which cmd.exe)
$CEXE \\/c "makecab /V2 /F $MCDIR /f $FLIST > $MCLOG"
if [[ ! -f $MCOUT ]]; then
  echo "ERR: cabinet creation failed."
  exit 1
fi

rm -f $FDIRS $FALL $FLIST $MCDIR $MCLOG

exit 0
