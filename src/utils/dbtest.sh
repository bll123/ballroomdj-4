#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

tcount=0
pass=0
fail=0
emsg=""

function updateCounts {
  trc=$1

  tcount=$(($tcount+1))
  if [[ $trc -eq 0 ]]; then
    pass=$(($pass+1))
  else
    fail=$(($fail+1))
  fi
}

function checkres {
  tname=$1
  r=$2
  e=$3

  if [[ $r == $e ]]; then
    trc=0
  else
    echo "         Got: $r"
    echo "    Expected: $e"
    trc=1
  fi
  return $trc
}

function compcheck {
  tname=$1
  retc=$2

  if [[ $retc -eq 0 ]]; then
    trc=0
  else
    echo "    comparison failed"
    trc=1
  fi
  return $trc
}

function checkaudiotags {
  tname=$1

  grc=0
  find test-music -print |
  while read f; do
    if [[ ! -f "$f" ]]; then
      continue
    fi
    ./bin/bdj4 --bdj4tags "$f" > $TMPA
    bf=$(echo "$f" | sed 's,^test-music/,,')
    ./bin/bdj4 --tdbdump data/musicdb.dat "$bf" > $TMPB
    diff $TMPA $TMPB > /dev/null 2>&1
    trc=$?
    if [[ $trc -ne 0 ]]; then
      echo "    audio tag check failed"
      grc=1
    fi
  done
  return $grc
}

function dispres {
  tname=$1
  rca=$2
  rcb=$3

  if [[ $rca -eq 0 && $rcb -eq 0 ]]; then
    echo "$tname OK"
  else
    echo "$tname FAIL"
    echo $msg
  fi
  msg=""
}

function setwritetagson {
  gconf=data/bdjconfig.txt
  sed -e '/^WRITETAGS/ { n ; s/.*/..ALL/ ; }' \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function setbdj3compaton {
  gconf=data/bdjconfig.txt
  sed -e '/^BDJ3COMPATTAGS$/ { n ; s/.*/..yes/ ; }' \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function setbdj3compatoff {
  gconf=data/bdjconfig.txt
  sed -e '/^BDJ3COMPATTAGS$/ { n ; s/.*/..no/ ; }' \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function cleanallaudiofiletags {
  for f in test-music/*; do
    ./bin/bdj4 --bdj4tags --cleantags --quiet "$f"
  done
}

function setorgregex {
  org=$1

  gconf=data/bdjconfig.txt
  sed -e "/^ORGPATH/ { n ; s~.*~..${org}~ ; }" \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

ATIBDJ4=F
for arg in "$@"; do
  case $arg in
    --atibdj4ro)
      ATIBDJ4=RO
      ;;
    --atibdj4)
      ATIBDJ4=T
      ;;
  esac
done

# debug level
DBG=4456459
# norm
NUMM=134
# deleted foxtrot
NUMC=$(($NUMM-6))
NUMB=15
NUMBL1=$(($NUMB-1))
NUMR=13

DATADB=data/musicdb.dat
TMAINDB=test-templates/musicdb.dat
INMAINDB=test-templates/test-music.txt
INNOCHACHA=test-templates/test-m-nochacha.txt
INCHACHA=test-templates/test-m-chacha.txt
INNOFOXTROT=test-templates/test-m-nofoxtrot.txt
INCOMPAT=tmp/test-music-compat.txt
INR=test-templates/test-m-regex.txt
INRDAT=test-templates/test-m-regex-dat.txt
INRDT=test-templates/test-m-regex-dt.txt
INRDTAT=test-templates/test-m-regex-dtat.txt
TDBNOCHACHA=tmp/test-m-nochacha.dat
TDBNOFOXTROT=tmp/test-m-nofoxtrot.dat
TDBCHACHA=tmp/test-m-chacha.dat
TDBEMPTY=tmp/test-m-empty.dat
TDBCOMPACT=tmp/test-m-compact.dat
TDBCOMPAT=tmp/test-m-compat.dat
TDBRDAT=tmp/test-m-r-dat.dat
TDBRDT=tmp/test-m-r-dt.dat
TDBRDTALT=tmp/test-m-r-dt-alt.dat
TMDT=tmp/test-music-dt
TDBRDTAT=tmp/test-m-r-dtat.dat
TMSONGEND=test-music/037-all-tags-mp3-a.mp3

TMPA=tmp/dbtesta.txt
TMPB=tmp/dbtestb.txt

echo "## make test setup"
ATIFLAG=""
if [[ $ATIBDJ4 == T ]]; then
  ATIFLAG=--atibdj4
fi
./src/utils/mktestsetup.sh --force --debug ${DBG} ${ATIFLAG}

if [[ $ATIBDJ4 == T || $ATIBDJ4 == RO ]]; then
  ATII=libatibdj4
  hostname=$(hostname)
  tfn=data/${hostname}/bdjconfig.txt
  sed -e "/^AUDIOTAG/ { n ; s,.*,..${ATII}, ; }" \
      ${tfn} > ${tfn}.n
  mv -f ${tfn}.n ${tfn}
fi

# get music dir
hostname=$(hostname)
mconf=data/${hostname}/bdjconfig.txt
musicdir=$(sed -n -e '/^DIRMUSIC/ { n; s/^\.\.//; p ; }' $mconf)

TESTON=T

if [[ $TESTON == T ]]; then
  # main test db : rebuild of standard test database
  tname=rebuild-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --rebuild \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMM} skip 0 indb 0 new ${NUMM} updated 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TMAINDB)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $crc)
  dispres $tname $rc $crc
fi

if [[ $TESTON == T ]]; then
  # main test db : check-new with no changes
  tname=checknew-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --checknew \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMM} skip ${NUMM} indb ${NUMM} new 0 updated 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TMAINDB)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $rc)
  dispres $tname $rc $crc
fi

if [[ $TESTON == T ]]; then
  # main test db : update-from-tags with no changes
  tname=updfromtags-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --updfromtags \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMM} skip 0 indb ${NUMM} new 0 updated ${NUMM} notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TMAINDB)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $rc)
  dispres $tname $rc $crc
fi

if [[ $TESTON == T ]]; then
  # main test db : compact with no changes
  tname=compact-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --compact \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMM} skip 0 indb ${NUMM} new 0 updated ${NUMM} notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TMAINDB)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $rc)
  dispres $tname $rc $crc
fi

# restore the main test database, needed for write tags check
# only the main db has songs with song-start/song-end/vol-adjust-perc
cp -f $TMAINDB $DATADB

if [[ $TESTON == T ]]; then
  # test db : write tags
  # note that if the ati interface can't write tags, no changes are made
  # to the audio files, and everything will still look ok.
  # this test may look ok, as the default is to have bdj3 compatibility on
  tname=writetags-bdj3-compat-on
  setwritetagson
  setbdj3compaton
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --writetags \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMM} skip 0 indb ${NUMM} new 0 updated 0 notaudio 0 writetag ${NUMM}"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TMAINDB)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $crc)

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    msg+=$(checkaudiotags $tname)
    trc=$?
    updateCounts $trc
    if [[ $trc -ne 0 ]]; then
      rc=$trc
    fi
  fi

  # check one of the files
  val=$(python3 scripts/mutagen-inspect "${TMSONGEND}" | grep SONGEND)
  case ${val} in
    TXXX=SONGEND=0:29.0)
      ;;
    *)
      msg+="audio tags not written"
      rc=1
      ;;
  esac
  dispres $tname $rc $crc
fi

# create test db w/different song-end
# the problem here is that with bdj3 compatibility off, there is no
# forced rewrite of the tags, therefore to test, the value must be
# different
cp -f $INMAINDB $INCOMPAT
tfn=$INCOMPAT
sed -e '/^SONGEND/ { n ; s/.*/..28000/ ; }' \
    ${tfn} > ${tfn}.n
mv -f ${tfn}.n ${tfn}
./src/utils/mktestsetup.sh \
    --infile $INCOMPAT \
    --outfile $TDBCOMPAT \
    --debug ${DBG} ${ATIFLAG} \
    --keepmusic
# want the fixed version after the udpater has run
cp -f $DATADB $TDBCOMPAT

if [[ $TESTON == T ]]; then
  # test db : write tags
  # note that if the ati interface can't write tags, no changes are made
  # to the audio files, and everything will still look ok.
  tname=writetags-bdj3-compat-off
  setwritetagson
  setbdj3compatoff
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --writetags \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMM} skip 0 indb ${NUMM} new 0 updated 0 notaudio 0 writetag ${NUMM}"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBCOMPAT)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $crc)

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    msg+=$(checkaudiotags $tname)
    trc=$?
    updateCounts $trc
    if [[ $trc -ne 0 ]]; then
      rc=$trc
    fi
  fi

  # check one of the files
  val=$(python3 scripts/mutagen-inspect "${TMSONGEND}" | grep SONGEND)
  case ${val} in
    TXXX=SONGEND=28000)
      ;;
    *)
      msg+="audio tags not written"
      rc=1
      ;;
  esac
  dispres $tname $rc $crc
fi

# the tags are now incorrect due to the prior test.
# re-create the main database and test music
./src/utils/mktestsetup.sh --force --debug ${DBG} ${ATIFLAG}

# clean any leftover foxtrot from the tmp dir
rm -f tmp/*-foxtrot.mp3
# save all foxtrot
mv -f test-music/*-foxtrot.mp3 tmp

# create test setup/db w/no foxtrot
./src/utils/mktestsetup.sh \
    --infile $INNOFOXTROT \
    --outfile $TDBNOFOXTROT \
    --debug ${DBG} ${ATIFLAG}

# restore the main database
cp -f $TMAINDB $DATADB

if [[ $TESTON == T ]]; then
  # main test db : check-new with deleted files
  tname=checknew-delete
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --checknew \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  # note that the music database still has the entries for the
  # deleted files in it.
  exp="found ${NUMC} skip ${NUMC} indb ${NUMC} new 0 updated 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBNOFOXTROT)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $rc)
  dispres $tname $rc $crc
fi

# restore the main database
cp -f $TMAINDB $DATADB

if [[ $TESTON == T ]]; then
  # main test db : compact with deleted files
  tname=compact-deleted
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --compact \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMC} skip 0 indb ${NUMC} new 0 updated ${NUMC} notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBNOFOXTROT)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $rc)
  dispres $tname $rc $crc
fi

# restore all foxtrot
mv -f tmp/*-foxtrot.mp3 test-music

# create test db w/no data
./src/utils/mktestsetup.sh \
    --emptydb \
    --infile $INCHACHA \
    --outfile $TDBEMPTY \
    --debug ${DBG} ${ATIFLAG}
# create test db w/chacha
./src/utils/mktestsetup.sh \
    --infile $INCHACHA \
    --outfile $TDBCHACHA \
    --debug ${DBG} ${ATIFLAG}
# save the cha cha
rm -f tmp/001-chacha.mp3
mv -f test-music/001-chacha.mp3 tmp
# create test db w/o chacha
# will copy tmp/test-m-b.dat to data/
./src/utils/mktestsetup.sh \
    --infile $INNOCHACHA \
    --outfile $TDBNOCHACHA \
    --debug ${DBG} ${ATIFLAG}

if [[ $TESTON == T ]]; then
  # test db : rebuild of test-m-nochacha
  if [[ -f test-music/001-chacha.mp3 ]]; then
    echo "cha cha present when it should not be"
    rc=1
    crc=0
    dispres $tname $rc $crc
  else
    tname=rebuild-test-db
    got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbtopdir "${musicdir}" \
      --cli --wait --verbose)
    exp="found ${NUMBL1} skip 0 indb 0 new ${NUMBL1} updated 0 notaudio 0 writetag 0"
    msg+=$(checkres $tname "$got" "$exp")
    rc=$?
    updateCounts $rc
    msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBNOCHACHA)
    crc=$?
    updateCounts $crc
    msg+=$(compcheck $tname $rc)
    dispres $tname $rc $crc
  fi
fi

# restore the cha cha
mv -f tmp/001-chacha.mp3 test-music

if [[ $TESTON == T ]]; then
  # test db : check-new w/cha cha
  tname=checknew-chacha
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --checknew \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMB} skip ${NUMBL1} indb ${NUMBL1} new 1 updated 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBCHACHA)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $crc)
  dispres $tname $rc $crc
fi

# clean all of the tags from the music files
cleanallaudiofiletags

if [[ $TESTON == T ]]; then
  # test db : rebuild with no tags
  tname=rebuild-no-tags
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --rebuild \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMB} skip 0 indb 0 new ${NUMB} updated 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  # no db comparison
  dispres $tname $rc $rc
fi

# restore the empty database needed for update from tags check
cp -f $TDBEMPTY $DATADB

if [[ $TESTON == T ]]; then
  # test db : update from tags
  tname=update-from-tags-empty-db
  setwritetagson
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --updfromtags \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMB} skip 0 indb ${NUMB} new 0 updated ${NUMB} notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBEMPTY)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $crc)

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    msg+=$(checkaudiotags $tname)
    trc=$?
    updateCounts $trc
    if [[ $trc -ne 0 ]]; then
      rc=$trc
    fi
  fi
  dispres $tname $rc $crc
fi

# restore the cha cha database
cp -f $TDBCHACHA $DATADB

if [[ $TESTON == T ]]; then
  # test db : write tags
  tname=write-tags
  setwritetagson
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --writetags \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMB} skip 0 indb ${NUMB} new 0 updated 0 notaudio 0 writetag ${NUMB}"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBCHACHA)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $crc)

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    msg+=$(checkaudiotags $tname)
    trc=$?
    updateCounts $trc
    if [[ $trc -ne 0 ]]; then
      rc=$trc
    fi
  fi
  dispres $tname $rc $crc
fi

# create test regex db w/tags (dance/artist - title)
./src/utils/mktestsetup.sh \
    --infile $INRDAT \
    --outfile $TDBRDAT \
    --debug ${DBG} ${ATIFLAG}
# create test regex db w/tags (dance/title)
./src/utils/mktestsetup.sh \
    --infile $INRDT \
    --outfile $TDBRDT \
    --debug ${DBG} ${ATIFLAG}
# create test regex db w/tags (dance/title) and w/alternate entries
tdir=$(echo ${musicdir} | sed 's,/test-music.*,,')
./src/utils/mktestsetup.sh \
    --infile $INRDT \
    --outfile $TDBRDTALT \
    --altdir ${tdir}/$TMDT \
    --debug ${DBG} ${ATIFLAG}
# create test regex db w/tags (dance/tn-artist - title)
./src/utils/mktestsetup.sh \
    --infile $INRDTAT \
    --outfile $TDBRDTAT \
    --debug ${DBG} ${ATIFLAG}
# create test music dir w/o any tags, tmpa is not used
./src/utils/mktestsetup.sh \
    --infile $INR \
    --outfile $TMPA \
    --debug ${DBG} ${ATIFLAG}
# copy the test-music to an alternate folder for testing
# secondary folder builds
test -d $TMDT && rm -rf $TMDT
cp -r test-music $TMDT

if [[ $TESTON == T ]]; then
  # test regex db : get dance/artist/title from file path
  tname=rebuild-file-path-dat
  setorgregex '{%DANCE%/}{%ARTIST% - }{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --rebuild \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMR} skip 0 indb 0 new ${NUMR} updated 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBRDAT)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $crc)
  dispres $tname $rc $crc
fi

if [[ $TESTON == T ]]; then
  # test regex db : get dance/title from file path
  tname=rebuild-file-path-dt
  setorgregex '{%DANCE%/}{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --rebuild \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMR} skip 0 indb 0 new ${NUMR} updated 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBRDT)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $crc)
  dispres $tname $rc $crc
fi

if [[ $TESTON == T ]]; then
  # test secondary folder: regex db : get dance/title from file path
  tname=rebuild-file-path-dt-alt
  tdir=$(echo ${musicdir} | sed 's,/test-music.*,,')
  setorgregex '{%DANCE%/}{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --checknew \
    --dbtopdir "${tdir}/${TMDT}" \
    --cli --wait --verbose)
  exp="found ${NUMR} skip 0 indb 0 new ${NUMR} updated 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBRDTALT)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $crc)
  dispres $tname $rc $crc
fi

if [[ $TESTON == T ]]; then
  # test regex db : get date/tracknum-artist-title from file path
  tname=rebuild-file-path-dtat
  setorgregex '{%DANCE%/}{%TRACKNUMBER0%-}{%ARTIST% - }{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
    --debug ${DBG} \
    --rebuild \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMR} skip 0 indb 0 new ${NUMR} updated 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+=$(./bin/bdj4 --tdbcompare $DATADB $TDBRDTAT)
  crc=$?
  updateCounts $crc
  msg+=$(compcheck $tname $crc)
  dispres $tname $rc $crc
fi

# remove test db, temporary files
rm -f $TDBNOCHACHA $TDBCHACHA $TDBEMPTY $TDBCOMPACT
rm -f $TDBRDAT $TDBRDT $TDBRDTALT $TDBRDTAT
rm -f $TMPA $TMPB
rm -rf $TMDT
# clean any leftover foxtrot
rm -f tmp/*-foxtrot.mp3

echo "tests: $tcount pass: $pass fail: $fail"
rc=1
if [[ $tcount -eq $pass ]]; then
  echo "==final OK"
  rc=0
else
  echo "==final FAIL"
fi

exit $rc
