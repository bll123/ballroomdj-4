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

if [[ $1 == --force ]]; then
  rm -f data/mktestdb.txt
fi
./src/utils/mktestdb.sh

if [[ ! -d data ]]; then
  exit 1
fi

rm -rf data
rm -rf img/profile0[1-9]

hostname=$(hostname)
mkdir -p data/profile00
mkdir -p data/${hostname}/profile00
touch data/mktestdb.txt

for f in templates/ds-*.txt; do
  cp -f $f data/profile00
done
for f in templates/*.txt; do
  case $f in
    *bdjconfig.txt*)
      continue
      ;;
    *ds-*.txt)
      continue
      ;;
  esac
  cp -f $f data
done
cp -f templates/bdjconfig.txt.g data/bdjconfig.txt
cp -f templates/bdjconfig.txt.p data/profile00/bdjconfig.txt
cp -f templates/bdjconfig.txt.m data/${hostname}/bdjconfig.txt
cp -f templates/bdjconfig.txt.mp data/${hostname}/profile00/bdjconfig.txt
cp -f templates/automatic.* data
cp -f templates/standardrounds.* data
cp -f test-templates/musicdb.dat data
cp -f test-templates/status.txt data
cp -f test-templates/ds-songfilter.txt data/profile00
cp -f test-templates/test-songlist.pl data
cp -f test-templates/test-songlist.pldances data
cp -f test-templates/test-songlist.songlist data
cp -f test-templates/test-sequence.pl data
cp -f test-templates/test-sequence.pldances data
cp -f test-templates/test-sequence.sequence data
cp -f test-templates/test-auto-a.pl data
cp -f test-templates/test-auto-a.pldances data

cwd=$(pwd)

ed data/profile00/bdjconfig.txt << _HERE_ > /dev/null
/^DEFAULTVOLUME/
+1
s,.*,..35,
/^FADEOUTTIME/
+1
s,.*,..4000,
/^HIDEMARQUEEONSTART/
+1
s,.*,..on,
/^PROFILENAME/
+1
s,.*,..Test-Setup,
w
q
_HERE_

ed data/${hostname}/bdjconfig.txt << _HERE_ > /dev/null
/^DIRMUSIC/
+1
s,.*,..${cwd}/test-music,
w
q
_HERE_

ed data/bdjconfig.txt << _HERE_ > /dev/null
/^DEBUGLVL/
+1
s,.*,..31,
w
q
_HERE_

for f in ds-history.txt ds-mm.txt ds-musicq.txt ds-ezsonglist.txt \
    ds-ezsongsel.txt ds-request.txt ds-songlist.txt ds-songsel.txt; do
  cat >> data/profile00/$f << _HERE_
KEYWORD
FAVORITE
_HERE_
done

cat >> data/profile00/ds-songedit-b.txt << _HERE_
FAVORITE
_HERE_

# make sure various variables are set appropriately.
./bin/bdj4 --bdj4updater --newinstall \
    --musicdir "${cwd}/test-music"

exit 0
