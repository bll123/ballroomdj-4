#!/bin/bash
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

if [[ ! -d tmp ]]; then
  mkdir tmp
fi

# with audiotag/dbupdate
# DBGLEVEL=$((1+2+8+4194304+262144))
# with songsel
# DBGLEVEL=$((1+2+8+64))
# with socket, process, progstate
# DBGLEVEL=$((1+2+4+8+512+32768+524288))
# standard
# DBGLEVEL=$((1+2+4+8))
# minimal
DBGLEVEL=1

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
  MINGW64*|CYGWIN*)
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

ATI=BDJ4
PLI=VLC3
VOL=
DBCOPY=T
KEEPDB=F
setpli=F
setvol=F
setati=F
# $@ must be preserved, it is passed on to mkdbtest.sh
for arg in "$@"; do
  case $arg in
    --ati)
      setati=T
      ;;
    --pli)
      # pli argument may be one of: VLC3, VLC4, GST, MPRISVLC, WINMP, MACAV
      setpli=T
      ;;
    --vol)
      # vol argument may be one of: pipewire
      setvol=T
      ;;
    --nodbcopy)
      DBCOPY=F
      ;;
    --keepdb)
      KEEPDB=T
      ;;
    *)
      if [[ $setpli == T ]]; then
        PLI=$arg
        setpli=F
      fi
      if [[ $setvol == T ]]; then
        VOL=$arg
        setvol=F
      fi
      if [[ $setati == T ]]; then
        ATI=$arg
        setati=F
      fi
      ;;
  esac
done

# copy this stuff before creating the database...
# otherwise the dances.txt file may be incorrect.

if [[ -f $FLAG ]]; then
  # preserve the flag file
  mv $FLAG .
fi
if [[ $KEEPDB == T ]]; then
  mv -f data/musicdb.dat .
fi
test -d data && rm -rf data
rm -rf img/profile0[1-9]

hostname=$(hostname)
mkdir -p data/profile00
mkdir -p data/${hostname}/profile00

if [[ $KEEPDB == T ]]; then
  mv musicdb.dat data
fi
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
    *bdjconfig.q?.txt)
      continue
      ;;
    *ds-*.txt)
      continue
      ;;
    *ui-*.txt)
      continue
      ;;
    *localization.txt|*html-list.txt|*helpdata.txt)
      continue
      ;;
  esac
  cp -f $f data
done
for f in templates/img/*.svg; do
  cp -f $f img/profile00
done

# for macos, copy the macos icons
if [[ $os == macos ]]; then
  for f in img/macos_icon*.svg img/macos_icon*.png; do
    case $f in
      *-base.svg)
        continue
        ;;
    esac
    tfn=$(echo $f | sed 's,macos,bdj4,')
    cp -f ${f} ${tfn}
  done
fi

cp -f templates/bdjconfig.txt.g data/bdjconfig.txt
cp -f templates/bdjconfig.txt.p data/profile00/bdjconfig.txt
for fn in templates/bdjconfig.q?.txt; do
  cp -f ${fn} data/profile00
done
cp -f templates/bdjconfig.txt.m data/${hostname}/bdjconfig.txt
cp -f templates/bdjconfig.txt.mp data/${hostname}/profile00/bdjconfig.txt
cp -f templates/QueueDance.* data
cp -f templates/standardrounds.* data
# the test dances data file has announcements set for tango & waltz
cp -f test-templates/dances.txt data
# the test status data file has an additional 'edit' status.
cp -f test-templates/status.txt data
cp -f test-templates/ds-songfilter.txt data/profile00
cp -f test-templates/ds-songedit-b.txt data/profile00
cp -f test-templates/ui-*.txt data/profile00
mv -f data/profile00/ui-starter.txt data
cp -f test-templates/audiosrc.txt data

for ftype in sl seq auto podcast; do
  for tag in a b c d e f g h; do
    if [[ $ftype == auto && $tag == f ]]; then
      break
    fi
    if [[ $ftype == seq && $tag == f ]]; then
      break
    fi
    if [[ $ftype == podcast && $tag == b ]]; then
      break
    fi
    copytestf ${ftype} ${tag} pl 1
    copytestf ${ftype} ${tag} pldances 0
    copytestf ${ftype} ${tag} songlist 0
    copytestf ${ftype} ${tag} sequence 0
    copytestf ${ftype} ${tag} podcast 0
  done
done

for tfn in data/profile00/bdjconfig.q?.txt; do
  sed -e '/^FADEOUTTIME/ { n ; s/.*/..4000/ ; }' \
      -e '/^GAP/ { n ; s/.*/..2000/ ; }' \
      ${tfn} > ${tfn}.n
  mv -f ${tfn}.n ${tfn}
done

tfn=data/profile00/bdjconfig.txt
sed -e '/^DEFAULTVOLUME/ { n ; s/.*/..20/ ; }' \
    -e '/^MARQUEE_SHOW/ { n ; s/.*/..minimize/ ; }' \
    -e '/^PROFILENAME/ { n ; s/.*/..Test-Setup/ ; }' \
    -e '/^UI_PROFILE_COL/ { n ; s/.*/..#0797ff/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

ATII=libatibdj4

PLII=libplivlc
PLIINM="Integrated VLC 3"
if [[ $PLI == VLC4 ]]; then
  PLII=libplivlc4
  PLIINM="Integrated VLC 4"
fi
if [[ $PLI == MPRISVLC ]]; then
  PLII=libplimpris
  PLIINM="MPRIS VLC Media Player"
fi
if [[ $PLI == GST ]]; then
  PLII=libpligst
  PLIINM="GStreamer"
fi
if [[ $PLI == WINMP ]]; then
  PLII=libpliwinmp
  PLIINM="Windows Media Player"
fi
if [[ $PLI == MACAV ]]; then
  PLII=libplimacosav
  PLIINM="MacOS AVPlayer"
fi

# if VOLI is empty, no change is made, and the default is used
VOLI=
if [[ $VOL == pipewire ]]; then
  VOLI=libvolpipewire
fi

tfn=data/${hostname}/bdjconfig.txt
tmusicdir="${cwd}/test-music"
titunes="${cwd}/test-files/iTunes-test-music.xml"
if [[ $platform == windows ]]; then
  tmusicdir=$(cygpath --mixed "${tmusicdir}")
  titunes=$(cygpath --mixed "${titunes}")
fi
sed \
    -e "/^AUDIOTAG/ { n ; s,.*,..${ATII}, ; }" \
    -e '/^DEFAULTVOLUME/ { n ; s/.*/..25/ ; }' \
    -e "/^DIRMUSIC/ { n ; s,.*,..${tmusicdir}, ; }" \
    -e "/^ITUNESXMLFILE/ { n ; s,.*,..${titunes}, ; }" \
    -e "/^PLAYER$/ { n ; s,.*,..${PLII}, ; }" \
    -e "/^PLAYER_I_NM/ { n ; s,.*,..${PLIINM}, ; }" \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}
if [[ $VOLI != "" ]]; then
  sed \
      -e "/^VOLUME$/ { n ; s,.*,..${VOLI}, ; }" \
      ${tfn} > ${tfn}.n
  mv -f ${tfn}.n ${tfn}
fi

tfn=data/bdjconfig.txt
sed -e "/^DEBUGLVL/ { n ; s/.*/..${DBGLEVEL}/ ; }" \
    -e '/^CLOCKDISP/ { n ; s/.*/..iso/ ; }' \
    -e '/^BPM/ { n ; s/.*/..BPM/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

if [[ $os == linux ]]; then
  tfn=data/${hostname}/bdjconfig.txt
  sed -e '/^MQ_FONT/ { n ; s/.*/..Yanone Kaffeesatz 12/ ; }' \
      -e '/^LISTING_FONT/ { n ; s/.*/..Noto Sans Regular 15/ ; }' \
      ${tfn} > ${tfn}.n
  mv -f ${tfn}.n ${tfn}
fi

if [[ $os == macos ]]; then
  tfn=data/${hostname}/bdjconfig.txt
  sed -e '/^UI_THEME/ { n ; s/.*/..Mojave-dark-solid/ ; }' \
      -e '/^MQ_FONT/ { n ; s/.*/..Arial Narrow Regular 17/ ; }' \
      -e '/^UI_FONT/ { n ; s/.*/..Arial Regular 17/ ; }' \
      -e '/^LISTING_FONT/ { n ; s/.*/..Arial Regular 16/ ; }' \
      ${tfn} > ${tfn}.n
  mv -f ${tfn}.n ${tfn}
fi

if [[ $platform == windows ]]; then
  tfn=data/${hostname}/bdjconfig.txt
  sed -e '/^UI_THEME/ { n ; s/.*/..Windows-10-Dark/ ; }' \
      -e '/^UI_FONT/ { n ; s/.*/..Arial Regular 14/ ; }' \
      -e '/^LISTING_FONT/ { n ; s/.*/..Arial Regular 13/ ; }' \
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

args=""

outfile=$(./src/utils/mktestdb.sh "$@")
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "FAIL: mktestdb failed"
  exit 1
fi
# if an outfile was specified,
# copy the db to the data dir after it is created
if [[ $DBCOPY == T ]]; then
  if [[ $outfile != "" ]]; then
    cp -f $outfile data/musicdb.dat
  fi
fi

# run with the newinstall flag to make sure various variables are
# set correctly.
# do not remove the updater.txt file...
# the fix-pl-disable-grp should not be run
./bin/bdj4 --bdj4updater --newinstall

# bdj4updater will change the itunes media dir
tfn=data/${hostname}/bdjconfig.txt

titunes="${cwd}/test-files"
if [[ $platform == windows ]]; then
  titunes=$(cygpath --mixed "${titunes}")
fi
sed -e "/^DIRITUNESMEDIA/ { n ; s,.*,..${titunes}, ; }" \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}
# bdj4updater will change the orgpath to the itunes orgpath.
tfn=data/bdjconfig.txt
sed -e '/^ORGPATH$/ { n ; s,.*,..{%DANCE%/}{%TITLE%}, ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}

exit 0
