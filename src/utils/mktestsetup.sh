#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

systype=$(uname -s)
case $systype in
  Linux)
    os=linux
    platform=unix
    ;;
  Darwin)
    os=macos
    platform=unix
    ;;
  MINGW64*)
    os=win64
    platform=windows
    ;;
  MINGW32*)
    os=win32
    platform=windows
    ;;
esac

FLAG=data/mktestdb.txt

# copy this stuff before creating the database...
# otherwise the dances.txt file may be incorrect.

if [[ -f $FLAG ]]; then
  # preserve the flag file
  mv $FLAG .
fi
test -d data && rm -rf data
rm -rf img/profile0[1-9]

hostname=$(hostname)
mkdir -p data/profile00
mkdir -p data/${hostname}/profile00

if [[ -f $(basename $FLAG) ]]; then
  # preserve the flag file
  mv $(basename $FLAG) $FLAG
fi

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
for q in 0 1 2 3; do
  cp -f templates/bdjconfig.q${q}.txt data/profile00/bdjconfig.q${q}.txt
done
cp -f templates/bdjconfig.txt.m data/${hostname}/bdjconfig.txt
cp -f templates/bdjconfig.txt.mp data/${hostname}/profile00/bdjconfig.txt
cp -f templates/automatic.* data
cp -f templates/QueueDance.* data
cp -f templates/standardrounds.* data
# the test dances data file has announcements set for tango & waltz
cp -f test-templates/dances.txt data
# the test status data file has an additional 'edit' status.
cp -f test-templates/status.txt data
cp -f test-templates/ds-songfilter.txt data/profile00
cp -f test-templates/ui-*.txt data/profile00

# songlist a
to=test-sl-a
cp -f test-templates/test-sl-a.pl data/${to}.pl
cp -f test-templates/test-sl-a.pldances data/${to}.pldances
cp -f test-templates/test-sl-a.songlist data/${to}.songlist
# songlist b
to=test-sl-b
cp -f test-templates/test-sl-a.pl data/${to}.pl
cp -f test-templates/test-sl-b.pldances data/${to}.pldances
cp -f test-templates/test-sl-a.songlist data/${to}.songlist
# songlist c
to=test-sl-c
cp -f test-templates/test-sl-c.pl data/${to}.pl
cp -f test-templates/test-sl-a.pldances data/${to}.pldances
cp -f test-templates/test-sl-a.songlist data/${to}.songlist
# songlist d
to=test-sl-d
cp -f test-templates/test-sl-d.pl data/${to}.pl
cp -f test-templates/test-sl-a.pldances data/${to}.pldances
cp -f test-templates/test-sl-a.songlist data/${to}.songlist
# songlist e
to=test-sl-e
cp -f test-templates/test-sl-e.pl data/${to}.pl
cp -f test-templates/test-sl-a.pldances data/${to}.pldances
cp -f test-templates/test-sl-a.songlist data/${to}.songlist
# songlist f
to=test-sl-f
cp -f test-templates/test-sl-f.pl data/${to}.pl
cp -f test-templates/test-sl-a.pldances data/${to}.pldances
cp -f test-templates/test-sl-a.songlist data/${to}.songlist
# songlist g
to=test-sl-g
cp -f test-templates/test-sl-g.pl data/${to}.pl
cp -f test-templates/test-sl-a.pldances data/${to}.pldances
cp -f test-templates/test-sl-a.songlist data/${to}.songlist

# sequence a
to=test-seq-a
cp -f test-templates/test-seq-a.pl data/${to}.pl
cp -f test-templates/test-seq-a.pldances data/${to}.pldances
cp -f test-templates/test-seq-a.sequence data/${to}.sequence
# sequence b
to=test-seq-b
cp -f test-templates/test-seq-a.pl data/${to}.pl
cp -f test-templates/test-seq-b.pldances data/${to}.pldances
cp -f test-templates/test-seq-a.sequence data/${to}.sequence
# sequence c
to=test-seq-c
cp -f test-templates/test-seq-a.pl data/${to}.pl
cp -f test-templates/test-seq-c.pldances data/${to}.pldances
cp -f test-templates/test-seq-a.sequence data/${to}.sequence
# sequence d
to=test-seq-d
cp -f test-templates/test-seq-d.pl data/${to}.pl
cp -f test-templates/test-seq-a.pldances data/${to}.pldances
cp -f test-templates/test-seq-a.sequence data/${to}.sequence

# auto a
to=test-auto-a
cp -f test-templates/test-auto-a.pl data/${to}.pl
cp -f test-templates/test-auto-a.pldances data/${to}.pldances
# auto b
to=test-auto-b
cp -f test-templates/test-auto-a.pl data/${to}.pl
cp -f test-templates/test-auto-b.pldances data/${to}.pldances
# auto c
to=test-auto-c
cp -f test-templates/test-auto-a.pl data/${to}.pl
cp -f test-templates/test-auto-c.pldances data/${to}.pldances

tfn=data/profile00/bdjconfig.q0.txt
sed -e '/^FADEOUTTIME/ { n ; s/.*/..4000/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

tfn=data/profile00/bdjconfig.txt
sed -e '/^DEFAULTVOLUME/ { n ; s/.*/..25/ ; }' \
    -e '/^HIDEMARQUEEONSTART/ { n ; s/.*/..on/ ; }' \
    -e '/^PROFILENAME/ { n ; s/.*/..Test-Setup/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

tfn=data/${hostname}/bdjconfig.txt
sed -e '/^DEFAULTVOLUME/ { n ; s/.*/..25/ ; }' \
    -e "/^DIRMUSIC/ { n ; s,.*,..${cwd}/test-music, ; }" \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

tfn=data/bdjconfig.txt
sed -e '/^DEBUGLVL/ { n ; s/.*/..31/ ; }' \
    -e '/^CLOCKDISP/ { n ; s/.*/..iso/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

if [[ $os == macos ]]; then
  tfn=data/${hostname}/profile00/bdjconfig.txt
  sed -e '/UI_THEME/ { n ; s/.*/..macOS-Mojave-dark/ ; }' \
      -e '/MQFONT/ { n ; s/.*/..Arial Narrow Regular 17/ ; }' \
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

args=""

outfile=$(./src/utils/mktestdb.sh "$@")
# copy the db to the data dir after it is created
if [[ $outfile != "" ]]; then
  cp -f $outfile data/musicdb.dat
else
  cp -f test-templates/musicdb.dat data
fi

cwd=$(pwd)

# make sure various variables are set appropriately.
./bin/bdj4 --msys --bdj4updater --newinstall \
    --musicdir "${cwd}/test-music"

if [[ $os == macos ]]; then
  # reset the debug level on macos back to 31
  tfn=data/bdjconfig.txt
  sed -e '/^DEBUGLVL/ { n ; s/.*/..31/ ; }' \
      ${tfn} > ${tfn}.n
  mv -f ${tfn}.n ${tfn}
fi

exit 0
