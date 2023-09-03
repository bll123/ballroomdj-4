#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

# with audiotag/dbupdate
# DBGLEVEL=$((1+2+4+8+4194304+262144))
DBGLEVEL=$((1+2+4+8))

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
    echo "Platform not supported"
    exit 1
    ;;
esac

FLAG=data/mktestdb.txt

function copytestf {
  ftype=$1
  tag=$2
  suffix=$3

  to=test-${ftype}-${tag}

  ftag=a
  if [[ -f test-templates/test-${ftype}-${tag}.${suffix} ]]; then
    ftag=$tag
  fi
  if [[ -f test-templates/test-${ftype}-${ftag}.${suffix} ]]; then
    cp -f test-templates/test-${ftype}-${ftag}.${suffix} data/${to}.${suffix}
  fi
}

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

ATIBDJ4=F
ATIMUTAGEN=F
for arg in "$@"; do
  case $arg in
    --atibdj4)
      ATIBDJ4=T
      ;;
    --atimutagen)
      ATIMUTAGEN=T
      ;;
  esac
done

for f in templates/ds-*.txt; do
  cp -f $f data/profile00
done
for f in templates/*.txt; do
  case $f in
    *bdjconfig.txt*)
      continue
      ;;
    *bdjconfig.q?.txt)
      continue
      ;;
    *ds-*.txt)
      continue
      ;;
    *ui-*.txt)
      continue
      ;;
  esac
  cp -f $f data
done
for f in templates/img/*.svg; do
  cp -f $f img/profile00
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
mv -f data/profile00/ui-starter.txt data

for ftype in sl seq auto; do
  for tag in a b c d e f g; do
    if [[ $ftype == auto && $tag == d ]]; then
      break
    fi
    if [[ $ftype == seq && $tag == e ]]; then
      break
    fi
    copytestf ${ftype} ${tag} pl 1
    copytestf ${ftype} ${tag} pldances 0
    copytestf ${ftype} ${tag} songlist 0
    copytestf ${ftype} ${tag} sequence 0
  done
done

for tfn in data/profile00/bdjconfig.q?.txt; do
  sed -e '/^FADEOUTTIME/ { n ; s/.*/..4000/ ; }' \
      -e '/^GAP/ { n ; s/.*/..2000/ ; }' \
      ${tfn} > ${tfn}.n
  mv -f ${tfn}.n ${tfn}
done

tfn=data/profile00/bdjconfig.txt
sed -e '/^DEFAULTVOLUME/ { n ; s/.*/..25/ ; }' \
    -e '/^MARQUEE_SHOW/ { n ; s/.*/..minimize/ ; }' \
    -e '/^PROFILENAME/ { n ; s/.*/..Test-Setup/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

ATII=libatimutagen
if [[ $ATIBDJ4 == T ]]; then
  ATII=libatibdj4
fi
if [[ $ATIMUTAGEN == T ]]; then
  ATII=libatimutagen
fi
tfn=data/${hostname}/bdjconfig.txt
sed -e '/^DEFAULTVOLUME/ { n ; s/.*/..25/ ; }' \
    -e "/^DIRMUSIC/ { n ; s,.*,..${cwd}/test-music, ; }" \
    -e "/^ITUNESXMLFILE/ { n ; s,.*,..${cwd}/test-files/iTunes-test-music.xml, ; }" \
    -e "/^AUDIOTAG/ { n ; s,.*,..${ATII}, ; }" \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

tfn=data/bdjconfig.txt
sed -e "/^DEBUGLVL/ { n ; s/.*/..${DBGLEVEL}/ ; }" \
    -e '/^CLOCKDISP/ { n ; s/.*/..iso/ ; }' \
    -e '/^BPM/ { n ; s/.*/..BPM/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

if [[ $os == macos ]]; then
  tfn=data/${hostname}/profile00/bdjconfig.txt
  sed -e '/UI_THEME/ { n ; s/.*/..Mojave-dark/ ; }' \
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

# run with the newinstall flag to make sure various variables are
# set correctly.
# remove the updater config to make sure all updates get run
rm -f data/updater.txt
./bin/bdj4 --bdj4updater --newinstall \
    --musicdir "${cwd}/test-music"
# run again w/o newinstall to perform the updates
./bin/bdj4 --bdj4updater --writetags

tfn=data/updater.txt
sed -e '/^FIX_AF_MPM/ { n ; s/.*/..0/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

if [[ $os == macos ]]; then
  # reset the debug level on macos back to 31
  tfn=data/bdjconfig.txt
  sed -e '/^DEBUGLVL/ { n ; s/.*/..31/ ; }' \
      ${tfn} > ${tfn}.n
  mv -f ${tfn}.n ${tfn}
fi

# bdj4updater will change the itunes media dir
tfn=data/${hostname}/bdjconfig.txt
sed -e "/^DIRITUNESMEDIA/ { n ; s,.*,..${cwd}/test-music, ; }" \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}
# bdj4updater will change the orgpath to the itunes orgpath.
tfn=data/bdjconfig.txt
sed -e '/^ORGPATH/ { n ; s,.*,..{%DANCE%/}{%TITLE%}, ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

exit 0
