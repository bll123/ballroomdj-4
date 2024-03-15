#!/bin/bash
#
# Copyright 2024 Brad Lanam Pleasant Hill CA
#

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "Usage: $0 <shortcut-name> <install-path> <working-dir> <profile-num>"
  exit 1
fi

scname=$1
tgtpath=$2
workdir=$3
profile=$4

if [[ ! -d "$tgtpath" ]]; then
  echo "Could not locate $tgtpath"
  exit 1
fi

profargs=""
if [[ $profile != "" && $profile -gt 0 ]]; then
  profargs=" --profile $profile"
fi

desktop="$HOME/Desktop"
happl="$HOME/Applications"
apppfx="/Contents/MacOS"
binfn=bdj4.sh

app="${happl}/${scname}.app"
appmain="${app}${apppfx}"

test -d "${appmain}" || mkdir -p "${appmain}"

cd "${app}/Contents"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "cd to ${app}/Contents Failed"
fi
test -d Resources || mkdir Resources
# tgtpath is pointing to .../BDJ4.app/Contents/MacOS/
cp -pf "${tgtpath}/../Info.plist" .
cp -pf "${tgtpath}/../Pkginfo" .
cp -pf "${tgtpath}/../Resources/"* Resources

cd "${appmain}"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "cd to ${appmain} Failed"
fi

cp -pf "${tgtpath}/VERSION.txt" .
test -d bin || mkdir bin

cat > bin/${binfn} << _HERE_
#!/bin/bash

cd '${workdir}'
if [[ \$? -eq 0 ]]; then
  '${tgtpath}/bin/bdj4g' ${profargs} &
fi
exit 0
_HERE_

chmod a+rx bin/${binfn}

ln -sf bin/${binfn} BDJ4
ln -sf "${app}" "$HOME/Desktop/${scname}"

exit 0
