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

PATH=$(pwd)/plocal/bin:$PATH

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
  if [[ -f core ]]; then
    echo "   core dump"
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
  flags=$2

  ./bin/bdj4 --ttagdbchk ${VERBOSE} --debug ${DBG} data/musicdb.dat ${flags}
  atrc=$?
  return $atrc
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
  mdir=$1

  find $mdir -type f -print0 |
      xargs -n1 -0 ./bin/bdj4 --bdj4tags --cleantags --quiet
}

function setorgregex {
  org=$1

  gconf=data/bdjconfig.txt
  sed -e "/^ORGPATH/ { n ; s~.*~..${org}~ ; }" \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function checkreorg {
  tmdir=$1
  omdir=$2
  ddir=$3
  fn=$4

  trc=0

  if [[ $trc == 0 && -f "${tmdir}/${fn}" ]]; then
    echo "  ERR: Not renamed or incorrect: $fn"
    trc=1
  fi
  if [[ $trc == 0 && -f "${omdir}/${ddir}/${fn}" ]]; then
    echo "  ERR: Incorrect dir: $fn"
    trc=1
  fi
  if [[ $trc == 0 && ! -f "${tmdir}/${ddir}/${fn}" ]]; then
    echo "  ERR: Missing: $fn"
    trc=1
  fi

  return $trc
}


VERBOSE=""
ATIBDJ4=F
FIRSTONLY=F
EXITONFAIL=F
for arg in "$@"; do
  case $arg in
    --atibdj4)
      ATIBDJ4=T
      ;;
    --firstonly)
      FIRSTONLY=T
      ;;
    --verbose)
      VERBOSE=--verbose
      ;;
    --exitonfail)
      EXITONFAIL=T
      ;;
  esac
done

# debug level
# audiotag+dbupdate+info+basic+important
# DBG=4456459
# audiotag+dbupdate+info+basic+important + db
DBG=4457483
# norm
NUMNORM=137
# cha cha
NUMCC=16
# regex
NUMREGEX=13
# deleted foxtrot
NUMNOFT=$(($NUMNORM-6))
# deleted cha cha
NUMNOCC=$(($NUMCC-1))

# second
NUMSECOND=13
NUMSECONDTOT=150
# deleted foxtrot (only the second dir)
NUMSECONDNOFT=$(($NUMSECOND-1))
# deleted foxtrot (total)
NUMSECONDTOTNOFT=$(($NUMSECONDTOT-6-1))

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
TDBRDTSECOND=tmp/test-m-r-dt-second.dat
TDBRDTAT=tmp/test-m-r-dtat.dat
TMSONGEND=test-music/037-all-tags-mp3-a.mp3

TMPA=tmp/dbtesta.txt
TMPB=tmp/dbtestb.txt
TMPDIRA=tmp/ka
TMPDIRB=tmp/kb
TMPDIRDT=tmp/test-music-dt

INSECOND=test-templates/test-m-second.txt
INSECONDNOFT=test-templates/test-m-second-noft.txt
TDBSECOND=tmp/test-m-second.dat
TDBSECONDNOFT=tmp/test-m-second-noft.dat
TDBSECONDEMPTY=tmp/test-m-second-empty.dat
KDBSECOND=tmp/second-db.dat
# must use full path
SECONDMUSICDIR=$(pwd)/tmp/music-second

echo "## make test setup"
ATIFLAG=""
if [[ $ATIBDJ4 == T ]]; then
  ATIFLAG=--atibdj4
fi
./src/utils/mktestsetup.sh --force --debug ${DBG} ${ATIFLAG}

if [[ $ATIBDJ4 == T ]]; then
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
musicdir="$(sed -n -e '/^DIRMUSIC/ { n; s/^\.\.//; p ; }' $mconf)"

TESTON=T

if [[ $TESTON == T ]]; then
  # main test db : rebuild of standard test database
  tname=rebuild-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  exp="found ${NUMNORM} skip 0 indb 0 new ${NUMNORM} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TMAINDB)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
fi

if [[ $FIRSTONLY == T ]]; then
  TESTON=F
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # main test db : check-new with no changes
  tname=checknew-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  exp="found ${NUMNORM} skip ${NUMNORM} indb ${NUMNORM} new 0 updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TMAINDB)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # main test db : update-from-tags with no changes
  tname=update-from-tags-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --updfromtags \
      --cli --wait --verbose)
  exp="found ${NUMNORM} skip 0 indb ${NUMNORM} new 0 updated ${NUMNORM} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TMAINDB)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # main test db : compact with no changes
  tname=compact-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --compact \
      --cli --wait --verbose)
  exp="found ${NUMNORM} skip 0 indb ${NUMNORM} new 0 updated ${NUMNORM} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TMAINDB)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # restore the main test database, needed for write tags check
  # only the main db has songs with song-start/song-end/vol-adjust-perc
  cp -f $TMAINDB $DATADB

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
      --cli --wait --verbose)
  exp="found ${NUMNORM} skip 0 indb ${NUMNORM} new 0 updated 0 renamed 0 norename 0 notaudio 0 writetag ${NUMNORM}"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TMAINDB)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    msg+="$(checkaudiotags $tname)"
    trc=$?
    updateCounts $trc
    if [[ $trc -ne 0 ]]; then
      rc=$trc
    fi
  fi

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    # check one of the files with all tags
    val=""
    val=$(./bin/bdj4 --bdj4tags "${TMSONGEND}" | grep SONGEND)
    # this change was made after mutagen was removed.
    # this returns the BDJ4 value, and what is wanted here is the
    # raw value so that it can be seen that it is
    # set to TXXX=SONGEND=0:29.0
    case ${val} in
      SONGEND*29000)
        ;;
      *)
        msg+="audio tags not written"
        rc=1
        ;;
    esac
    updateCounts $rc
  fi
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
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
  # want the fixed version after the updater has run
  cp -f $DATADB $TDBCOMPAT

  # test db : write tags
  # note that if the ati interface can't write tags, no changes are made
  # to the audio files, and everything will still look ok.
  tname=writetags-bdj3-compat-off
  setwritetagson
  setbdj3compatoff
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --writetags \
      --cli --wait --verbose)
  exp="found ${NUMNORM} skip 0 indb ${NUMNORM} new 0 updated 0 renamed 0 norename 0 notaudio 0 writetag ${NUMNORM}"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBCOMPAT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    msg+="$(checkaudiotags $tname)"
    trc=$?
    updateCounts $trc
    if [[ $trc -ne 0 ]]; then
      rc=$trc
    fi
  fi

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    # check one of the files with all tags
    val=""
    val=$(./bin/bdj4 --bdj4tags "${TMSONGEND}" | grep SONGEND)
    case ${val} in
      SONGEND*28000)
        ;;
      *)
        msg+="audio tags not written"
        rc=1
        ;;
    esac
    updateCounts $rc
  fi
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # the tags are now incorrect due to the prior test.
  # re-create the main database and test music
  ./src/utils/mktestsetup.sh --force --debug ${DBG} ${ATIFLAG}

  # clean any leftover foxtrot from the tmp dir
  rm -rf $TMPDIRA
  mkdir $TMPDIRA
  # save all foxtrot
  mv -f $musicdir/*-foxtrot.mp3 $TMPDIRA

  # create test setup/db w/no foxtrot
  ./src/utils/mktestsetup.sh \
      --infile $INNOFOXTROT \
      --outfile $TDBNOFOXTROT \
      --debug ${DBG} ${ATIFLAG}

  # restore the main database
  cp -f $TMAINDB $DATADB

  # main test db : check-new with deleted files
  tname=checknew-delete
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  # note that the music database still has the entries for the
  # deleted files in it.
  exp="found ${NUMNOFT} skip ${NUMNOFT} indb ${NUMNOFT} new 0 updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBNOFOXTROT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # restore the main database
  cp -f $TMAINDB $DATADB

  # main test db : compact with deleted files
  tname=compact-deleted
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --compact \
      --cli --wait --verbose)
  exp="found ${NUMNOFT} skip 0 indb ${NUMNOFT} new 0 updated ${NUMNOFT} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBNOFOXTROT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # restore all foxtrot
  mv -f $TMPDIRA/* $musicdir

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
  rm -f $TMPDIRA/001-chacha.mp3
  mv -f $musicdir/001-chacha.mp3 $TMPDIRA
  # create test db w/o chacha
  # will copy tmp/test-m-b.dat to data/
  ./src/utils/mktestsetup.sh \
      --infile $INNOCHACHA \
      --outfile $TDBNOCHACHA \
      --debug ${DBG} ${ATIFLAG}

  # test db : rebuild of test-m-nochacha
  if [[ -f $musicdir/001-chacha.mp3 ]]; then
    echo "cha cha present when it should not be"
    rc=1
    crc=0
    dispres $tname $rc $crc
  else
    tname=rebuild-test-db
    got=$(./bin/bdj4 --bdj4dbupdate \
        --debug ${DBG} \
        --rebuild \
        --dbupmusicdir "${musicdir}" \
        --cli --wait --verbose)
    exp="found ${NUMNOCC} skip 0 indb 0 new ${NUMNOCC} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
    msg+=$(checkres $tname "$got" "$exp")
    rc=$?
    updateCounts $rc
    msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBNOCHACHA)"
    crc=$?
    updateCounts $crc
    msg+="$(compcheck $tname $rc)"
    dispres $tname $rc $crc
  fi
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # restore the cha cha
  mv -f $TMPDIRA/001-chacha.mp3 $musicdir

  # test db : check-new w/cha cha
  tname=checknew-chacha
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  exp="found ${NUMCC} skip ${NUMNOCC} indb ${NUMNOCC} new 1 updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBCHACHA)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # clean all of the tags from the music files
  cleanallaudiofiletags $musicdir

  # test db : rebuild with no tags
  tname=rebuild-no-tags
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  exp="found ${NUMCC} skip 0 indb 0 new ${NUMCC} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  # no db comparison
  dispres $tname $rc $rc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # restore the empty database needed for update from tags check
  cp -f $TDBEMPTY $DATADB

  # test db : update from tags
  tname=update-from-tags-empty-db
  setwritetagson
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --updfromtags \
      --cli --wait --verbose)
  exp="found ${NUMCC} skip 0 indb ${NUMCC} new 0 updated ${NUMCC} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBEMPTY)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    msg+="$(checkaudiotags $tname --ignoremissing)"
    trc=$?
    updateCounts $trc
    if [[ $trc -ne 0 ]]; then
      rc=$trc
    fi
  fi
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # restore the cha cha database
  cp -f $TDBCHACHA $DATADB

  # test db : write tags
  tname=write-tags
  setwritetagson
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --writetags \
      --cli --wait --verbose)
  exp="found ${NUMCC} skip 0 indb ${NUMCC} new 0 updated 0 renamed 0 norename 0 notaudio 0 writetag ${NUMCC}"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBCHACHA)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    msg+="$(checkaudiotags $tname)"
    trc=$?
    updateCounts $trc
    if [[ $trc -ne 0 ]]; then
      rc=$trc
    fi
  fi
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
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
  # create test regex db w/tags (dance/title) and w/secondary entries
  tdir="$(dirname ${musicdir})"
  ./src/utils/mktestsetup.sh \
      --infile $INRDT \
      --outfile $TDBRDTSECOND \
      --seconddir "${tdir}/$TMPDIRDT" \
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
  # copy the music-dir to an alternate folder for testing
  # secondary folder builds
  test -d $TMPDIRDT && rm -rf $TMPDIRDT
  cp -r $musicdir $TMPDIRDT

  # test regex db : get dance/artist/title from file path
  tname=rebuild-file-path-dat
  setorgregex '{%DANCE%/}{%ARTIST% - }{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  exp="found ${NUMREGEX} skip 0 indb 0 new ${NUMREGEX} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBRDAT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # test regex db : get dance/title from file path
  tname=rebuild-file-path-dt
  setorgregex '{%DANCE%/}{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  exp="found ${NUMREGEX} skip 0 indb 0 new ${NUMREGEX} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBRDT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # test secondary folder: regex db : get dance/title from file path
  tname=rebuild-file-path-dt-second
  tdir="$(dirname ${musicdir})"
  setorgregex '{%DANCE%/}{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${tdir}/${TMPDIRDT}" \
      --cli --wait --verbose)
  exp="found ${NUMREGEX} skip 0 indb 0 new ${NUMREGEX} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBRDTSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # test regex db : get date/tracknum-artist-title from file path
  tname=rebuild-file-path-dtat
  setorgregex '{%DANCE%/}{%TRACKNUMBER0%-}{%ARTIST% - }{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  exp="found ${NUMREGEX} skip 0 indb 0 new ${NUMREGEX} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBRDTAT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  test -d $SECONDMUSICDIR || mkdir -p $SECONDMUSICDIR

  # re-create both the main and second music dir

  ./src/utils/mktestsetup.sh --force --debug ${DBG} ${ATIFLAG}

  ./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbupmusicdir "${musicdir}" \
      --cli --wait

  ./src/utils/mktestsetup.sh \
      --infile $INSECOND \
      --outfile $TDBSECOND \
      --debug ${DBG} ${ATIFLAG} \
      --dbupmusicdir $SECONDMUSICDIR \
      --nodbcopy \
      --keepdb

  # second test db : check-new with a second dir
  tname=second-checknew
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${SECONDMUSICDIR}" \
      --cli --wait --verbose)

  exp="found ${NUMSECOND} skip 0 indb 0 new ${NUMSECOND} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TMAINDB $TDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc

  # keep a copy of the combined main+second database.
  cp -f $DATADB $KDBSECOND
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

echo "## check ann"
exit 1

if [[ $TESTON == T ]]; then
  # main+second test db : compact with no changes
  # compact must iterate through the database, not just
  # the specified directory.
  tname=second-compact-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --compact \
      --cli --wait --verbose)
  exp="found ${NUMSECONDTOT} skip 0 indb ${NUMSECONDTOT} new 0 updated ${NUMSECONDTOT} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # clean any leftovers from the tmp dir
  rm -rf $TMPDIRA $TMPDIRB
  mkdir $TMPDIRA
  mkdir $TMPDIRB
  # save all foxtrot
  mv -f $musicdir/*-foxtrot.mp3 $TMPDIRA
  mv -f $SECONDMUSICDIR/*-foxtrot.mp3 $TMPDIRB

  ./src/utils/mktestsetup.sh \
      --infile $INSECONDNOFT \
      --outfile $TDBSECONDNOFT \
      --dbupmusicdir $SECONDMUSICDIR \
      --keepmusic \
      --debug ${DBG} ${ATIFLAG}

  # restore the main+second database
  cp -f $KDBSECOND $DATADB

  # main+second test db : check-new with deleted files
  tname=second-checknew-delete
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  # note that the music database still has the entries for the
  # deleted files in it.
  exp="found ${NUMNOFT} skip ${NUMNOFT} indb ${NUMNOFT} new 0 updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc

  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${SECONDMUSICDIR}" \
      --cli --wait --verbose)
  exp="found ${NUMSECONDNOFT} skip ${NUMSECONDNOFT} indb ${NUMSECONDNOFT} new 0 updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  # no db comparison
  dispres $tname $rc $rc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # restore the full main+second database
  cp -f $KDBSECOND $DATADB

  # main+second test db : compact with deleted files
  # compact must iterate through the database, not just
  # the specified directory.
  tname=second-compact-deleted
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --compact \
      --cli --wait --verbose)
  exp="found ${NUMSECONDTOTNOFT} skip 0 indb ${NUMSECONDTOTNOFT} new 0 updated ${NUMSECONDTOTNOFT} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBNOFOXTROT $TDBSECONDNOFT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc

  # restore all foxtrot
  mv -f $TMPDIRA/* $musicdir
  mv -f $TMPDIRB/* $SECONDMUSICDIR

  # restore the full main+second database
  cp -f $KDBSECOND $DATADB
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # clean all of the tags from the music files
  cleanallaudiofiletags $musicdir
  cleanallaudiofiletags $SECONDMUSICDIR

  # while there are no tags, create an empty database for future use.

  # test db : rebuild with no tags
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)

  # add the secondary music dir
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${SECONDMUSICDIR}" \
      --cli --wait --verbose)

  cp -f $DATADB $TDBSECONDEMPTY

  # restore the full main+second database for use in the write-tags test
  cp -f $KDBSECOND $DATADB

  # main+second db : write tags
  tname=second-write-tags
  setwritetagson
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --writetags \
      --cli --wait --verbose)
  exp="found ${NUMSECONDTOT} skip 0 indb ${NUMSECONDTOT} new 0 updated 0 renamed 0 norename 0 notaudio 0 writetag ${NUMSECONDTOT}"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"

  if [[ $rc -eq 0 && $crc -eq 0 ]]; then
    msg+="$(checkaudiotags $tname --ignoremissing)"
    trc=$?
    updateCounts $trc
    if [[ $trc -ne 0 ]]; then
      rc=$trc
    fi
  fi
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # restore the empty main+second database for update-from-tags
  cp -f $TDBSECONDEMPTY $DATADB

  # main+second empty db : write tags
  tname=second-update-from-tags-empty-db
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --updfromtags \
      --cli --wait --verbose)
  exp="found ${NUMSECONDTOT} skip 0 indb ${NUMSECONDTOT} new 0 updated ${NUMSECONDTOT} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
fi

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

if [[ $TESTON == T ]]; then
  # restore the main+second database
  cp -f $KDBSECOND $DATADB

  # main+second empty db : write tags
  tname=second-reorg-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUMSECONDTOT} skip 0 indb ${NUMSECONDTOT} new 0 updated 0 renamed ${NUMSECONDTOT} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc

  crc=0

### need to check to make sure everything got renamed properly

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=/home/bll/s/bdj4/test-music
  omdir=/home/bll/s/bdj4/tmp/music-second
  ddir="Announce"
  for fn in samba.mp3 waltz.mp3 tango.mp3; do
    checkreorg "$tmdir" "$omdir" "$ddir" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      crc=1
    fi
  done

  # music-dir
  tmdir=/home/bll/s/bdj4/test-music
  omdir=/home/bll/s/bdj4/tmp/music-second
  ddir="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg "$tmdir" "$omdir" "$ddir" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      crc=1
    fi
  done

  # secondary
  tmdir=/home/bll/s/bdj4/tmp/music-second
  omdir=/home/bll/s/bdj4/test-music
  ddir="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg "$tmdir" "$omdir" "$ddir" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      crc=1
    fi
  done

  dispres $tname $rc $crc
fi

exit 1

if [[ $EXITONFAIL == T && ( $rc -ne 0 || $crc -ne 0 ) ]]; then
  exit 1
fi

# remove test db, temporary files
rm -f $INCOMPAT
rm -f $TDBNOCHACHA $TDBCHACHA $TDBEMPTY $TDBCOMPACT $TDBCOMPAT $TDBNOFOXTROT
rm -f $TDBRDAT $TDBRDT $TDBRDTSECOND $TDBRDTAT
rm -f $TDBSECOND $KDBSECOND $TDBSECONDNOFT $TDBSECONDEMPTY
rm -f $TMPA $TMPB
rm -rf $TMPDIRDT $SECONDMUSICDIR $TMPDIRA $TMPDIRB

echo "tests: $tcount pass: $pass fail: $fail"
rc=1
if [[ $tcount -eq $pass ]]; then
  echo "==final OK"
  rc=0
else
  echo "==final FAIL"
fi

exit $rc
