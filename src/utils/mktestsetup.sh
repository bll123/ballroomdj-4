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

systype=$(uname -s)
arch=$(uname -m)
case $systype in
  Linux)
    os=linux
    platform=unix
    archtag=
    ;;
  Darwin)
    os=macos
    platform=unix
    case $arch in
      x86_64)
        archtag=-intel
        ;;
      arm64)
        archtag=-m1
        ;;
    esac
    ;;
  MINGW64*)
    os=win64
    platform=windows
    archtag=
    ;;
  MINGW32*)
    os=win32
    platform=windows
    archtag=
    ;;
esac

if [[ ! -d data ]]; then
  echo "No data dir"
  exit 1
fi

# copy this stuff before creating the database...
# otherwise the dances.txt file may be incorrect.

rm -rf data
rm -rf img/profile0[1-9]

hostname=$(hostname)
mkdir -p data/profile00
mkdir -p data/${hostname}/profile00

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
cp -f test-templates/test-sl-a.pl data
cp -f test-templates/test-sl-a.pldances data
cp -f test-templates/test-sl-a.songlist data
cp -f test-templates/test-sl-a.pl data/test-sl-b.pl
cp -f test-templates/test-sl-b.pldances data
cp -f test-templates/test-sl-a.songlist data/test-sl-b.songlist
cp -f test-templates/test-seq-a.pl data
cp -f test-templates/test-seq-a.pldances data
cp -f test-templates/test-seq-a.sequence data
cp -f test-templates/test-seq-a.pl data/test-seq-b.pl
cp -f test-templates/test-seq-b.pldances data
cp -f test-templates/test-seq-a.sequence data/test-seq-b.sequence
cp -f test-templates/test-auto-a.pl data
cp -f test-templates/test-auto-a.pldances data

tfn=data/profile00/bdjconfig.txt
sed -e '/^DEFAULTVOLUME/ { n ; s/.*/..25/ ; }' \
    -e '/^FADEOUTTIME/ { n ; s/.*/..4000/ ; }' \
    -e '/^HIDEMARQUEEONSTART/ { n ; s/.*/..on/ ; }' \
    -e '/^PROFILENAME/ { n ; s/.*/..Test-Setup/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

tfn=data/${hostname}/bdjconfig.txt
sed -e '/^DEFAULTVOLUME/ { n ; s/.*/..25/ ; }' \
    -e '/^DIRMUSIC/ { n ; s,.*,..${cwd}/test-music, ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

tfn=data/bdjconfig.txt
sed -e '/^DEBUGLVL/ { n ; s/.*/..31/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

if [[ $os == macos ]]; then
  tfn=data/${hostname}/profile00/bdjconfig.txt
  sed -e '/UI_THEME/ { n ; s/.*/..macOS-Mojave-dark/ ; }' \
      -e '/UIFONT/ { n ; s/.*/..Arial Regular 17/ ; }' \
      -e '/LISTINGFONT/ { n ; s/.*/..Arial Regular 16/ ; }' \
      ${tfn} > ${tfn}.n
  mv -f ${tfn}.n ${tfn}
fi

if [[ $platform == windows ]]; then
  tfn=data/${hostname}/profile00/bdjconfig.txt
  sed -e '/UI_THEME/ { n ; s/.*/..Windows-10-Dark/ ; }' \
      -e '/UIFONT/ { n ; s/.*/..Arial Regular 14/ ; }' \
      -e '/LISTINGFONT/ { n ; s/.*/..Arial Regular 13/ ; }' \
      ${tfn} > ${tfn}.n
  mv -f ${tfn}.n ${tfn}
fi

for f in ds-history.txt ds-mm.txt ds-musicq.txt ds-ezsonglist.txt \
    ds-ezsongsel.txt ds-request.txt ds-songlist.txt ds-songsel.txt; do
  tfn=data/profile00/$f
  if [[ -f $tfn ]]; then
    cat >> $tfn << _HERE_
KEYWORD
FAVORITE
_HERE_
  fi
done

cat >> data/profile00/ds-songedit-b.txt << _HERE_
FAVORITE
_HERE_

if [[ $1 == --force ]]; then
  rm -f data/mktestdb.txt
fi
./src/utils/mktestdb.sh

cwd=$(pwd)

# make sure various variables are set appropriately.
./bin/bdj4 --bdj4updater --newinstall \
    --musicdir "${cwd}/test-music"

exit 0
