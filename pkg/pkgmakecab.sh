#!/bin/bash

cwd=$(pwd)
FDIRS=find-dirs.txt
FALL=find-all.txt
FLIST=makecab.ddf
MCDIR=makecab.dir
MCOUT=bdj4-install.cab
MCLOG=makecab.log

echo "$(date +%T): begin"
> $FALL
find bdj4-install -type d -print > $FDIRS
echo "$(date +%T): dir listing"
for d in $(cat $FDIRS); do
  echo "=== $d" >> $FALL
  find $d -mindepth 1 -maxdepth 1 -type f -print >> $FALL
done
echo "$(date +%T): created file listing"

awk -v CWD=$cwd '
BEGIN { gsub (/\//, "\\", CWD); }

{
  if (/^=== /) {
    sub (/^=== /, "");
    print ".Set DestinationDir=" $0;
  } else {
    print "\"C:\\msys64" CWD "\\" $0 "\" /inf=no";
  }
}' $FALL \
> $FLIST
echo "$(date +%T): created .ddf file"

cat > $MCDIR << _HERE_
.New Cabinet
.Set DiskDirectory1=.
.Set CabinetNameTemplate=$MCOUT
.set GenerateInf=OFF
.Set Cabinet=ON
.Set Compress=ON
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

unix2dos $MCDIR
unix2dos $FLIST

set -x
# dcwd=$(cygpath -w ${cwd} | sed 's,\\,\\\\,g')
cmd.exe /c "makecab /V1 /F $MCDIR /f $FLIST > $MCLOG"

echo "$(date +%T): cabinet created"
rm -f $FDIRS $FALL $FLIST $MCDIR $MCLOG

exit 0
