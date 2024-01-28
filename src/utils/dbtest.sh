#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
  shift

  tgrc=0
  while test $# -gt 0; do
    trc=$1
    shift
    if [[ $trc -ne 0 ]]; then
      tgrc=1
    fi
  done

  if [[ $tgrc -eq 0 ]]; then
    echo "$tname OK"
  else
    echo "$tname FAIL"
    echo $msg
  fi
  msg=""
}

function exitonfail {
  if [[ $EXITONFAIL == F ]]; then
    return
  fi

  tgrc=0
  while test $# -gt 0; do
    trc=$1
    shift
    if [[ $trc -ne 0 ]]; then
      tgrc=1
    fi
  done

  if [[ $tgrc -ne 0 ]]; then
    exit 1
  fi
}

function setautoorgon {
  gconf=data/bdjconfig.txt
  sed -e '/^AUTOORGANIZE$/ { n ; s/.*/..yes/ ; }' \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function setautoorgoff {
  gconf=data/bdjconfig.txt
  sed -e '/^AUTOORGANIZE$/ { n ; s/.*/..no/ ; }' \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function setwritetagson {
  gconf=data/bdjconfig.txt
  sed -e '/^WRITETAGS/ { n ; s/.*/..ALL/ ; }' \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function setwritetagsoff {
  gconf=data/bdjconfig.txt
  sed -e '/^WRITETAGS/ { n ; s/.*/..OFF/ ; }' \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function cleanallaudiofiletags {
  mdir=$1

  find $mdir -type f -print0 |
      xargs -n1 -0 ./bin/bdj4 --bdj4tags --cleantags --quiet
}

function setorgpath {
  org=$1

  gconf=data/bdjconfig.txt
  sed -e "/^ORGPATH/ { n ; s~.*~..${org}~ ; }" \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function createoriginal {
  tdir=$1
  fn=$2

  tfn="${tdir}/${fn}"
  if [[ -f $tfn ]]; then
    cp -pf $tfn $tfn.original
  fi
  tfn="${tdir}/${dance}/${fn}"
  if [[ -f $tfn ]]; then
    cp -pf $tfn $tfn.original
  fi
}

function removeoriginal {
  tdir=$1
  fn=$2

  tfn="${tdir}/${fn}.original"
  if [[ -f $tfn ]]; then
    rm -f $tfn
  fi
  tfn="${tdir}/${dance}/${fn}.original"
  if [[ -f $tfn ]]; then
    rm -f $tfn
  fi
}

function createexisting {
  tdir=$1
  dance=$2
  fn=$3

  tfn="${tdir}/${fn}"
  ndir="${tdir}/${dance}"
  nfn="${ndir}/${fn}"
  if [[ -f $tfn ]]; then
    test -d "${ndir}" || mkdir "${ndir}"
    cp -pf $tfn $nfn
  fi
}

function checkreorg {
  type=$1
  tmdir=$2
  omdir=$3
  dance=$4
  fn=$5

  trc=0

  if [[ $trc == 0 && $type != title && -f "${tmdir}/${fn}" ]]; then
    echo "  ERR: Not renamed or incorrect: $fn"
    trc=1
  fi

  if [[ $type == dir || $type == ann ]]; then
    tfn="${omdir}/${dance}/${fn}"
  fi
  if [[ $type == title ]]; then
    tfn="${omdir}/${fn}"
  fi
  if [[ $type == dash ]]; then
    if [[ $dance != "" ]]; then
      tfn="${omdir}/${dance} - ${fn}"
    else
      tfn="${omdir}/${fn}"
    fi
  fi
  if [[ $trc == 0 && -f ${tfn} ]]; then
    echo "  ERR: Incorrect dir: $fn"
    trc=1
  fi

  if [[ $type == dir || $type == ann ]]; then
    tfn="${tmdir}/${dance}/${fn}"
  fi
  if [[ $type == title ]]; then
    tfn="${tmdir}/${fn}"
  fi
  if [[ $type == dash ]]; then
    if [[ $dance != "" ]]; then
      tfn="${tmdir}/${dance} - ${fn}"
    else
      tfn="${tmdir}/${fn}"
    fi
  fi
  if [[ $trc == 0 && ! -f ${tfn} ]]; then
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
NUMNORM=139
# cha cha
NUMCC=18
# regex
NUMREGEX=13
# deleted foxtrot
NUMFT=6
NUMNOFT=$(($NUMNORM-${NUMFT}))
# deleted cha cha
NUMNOCC=$(($NUMCC-1))

# second
NUM2=13
NUM2TOT=152
NUM2TOT_RN=147   # five announcements
NUM2_EXIST=2    # two existing
NUM2TOT_RN_EXIST=$((${NUM2TOT_RN}-${NUM2_EXIST}))
# deleted foxtrot (only the second dir)
NUM2_FT=1
NUM2_NOFT=$((${NUM2}-${NUM2_FT}))
# deleted foxtrot (total)
NUM2TOT_NOFT=$((${NUM2TOT}-${NUMFT}-${NUM2_FT}))
NUM2TOT_RN_NOFT=$((${NUM2TOT_RN}-${NUMFT}-${NUM2_FT}))
NUM2TOT_RN_FT=$((${NUMFT}+${NUM2_FT}))

DATADB=data/musicdb.dat
KDBMAIN=tmp/main-db.dat
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
KDBREORGNOFT=tmp/second-noft-db.dat
# must use full path
SECONDMUSICDIR=${cwd}/tmp/music-second

echo "## make test setup"
ATIFLAG=""
if [[ $ATIBDJ4 == T ]]; then
  ATIFLAG=--atibdj4
fi
./src/utils/mktestsetup.sh --force --debug ${DBG} ${ATIFLAG}
cp -pf $DATADB $KDBMAIN

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
  # after a rebuild, the db-loc-lock flags will be lost
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --noloclockchk --debug ${DBG} $DATADB $KDBMAIN)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc
fi

if [[ $FIRSTONLY == T ]]; then
  TESTON=F
fi

if [[ $TESTON == T ]]; then
  # restore the main database
  cp -pf $KDBMAIN $DATADB

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
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBMAIN)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc
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
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBMAIN)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc
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
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBMAIN)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc
fi

if [[ $TESTON == T ]]; then
  # create test db w/different song-end
  # there is no forced rewrite of the tags,
  # therefore to test, the value must be different
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
  tname=write-tags
  setwritetagson
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
  exitonfail $rc $crc
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
  cp -f $KDBMAIN $DATADB

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
  exitonfail $rc $crc
fi

if [[ $TESTON == T ]]; then
  # restore the main database
  cp -f $KDBMAIN $DATADB

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
  exitonfail $rc $crc
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
    # after a rebuild, the db-loc-lock flags will be lost
    msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --noloclockchk --debug ${DBG} $DATADB $TDBNOCHACHA)"
    crc=$?
    updateCounts $crc
    msg+="$(compcheck $tname $rc)"
    dispres $tname $rc $crc
  fi
  exitonfail $rc $crc
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
  # the db-loc-lock flags are still lost at this time
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --noloclockchk --debug ${DBG} $DATADB $TDBCHACHA)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc
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
  dispres $tname $rc
  exitonfail $rc
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
  # an empty database has no db-loc-lock flags
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --noloclockchk --debug ${DBG} $DATADB $TDBEMPTY)"
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
  exitonfail $rc $crc
fi

if [[ $TESTON == T ]]; then
  # restore the cha cha database
  cp -f $TDBCHACHA $DATADB

  # test db : write tags
  tname=write-tags-restored
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
  exitonfail $rc $crc
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
  setorgpath '{%DANCE%/}{%ARTIST% - }{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  exp="found ${NUMREGEX} skip 0 indb 0 new ${NUMREGEX} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  # after a rebuild, there are no db-loc-lock flags
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --noloclockchk --debug ${DBG} $DATADB $TDBRDAT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc
fi

if [[ $TESTON == T ]]; then
  # test regex db : get dance/title from file path
  tname=rebuild-file-path-dt
  setorgpath '{%DANCE%/}{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  exp="found ${NUMREGEX} skip 0 indb 0 new ${NUMREGEX} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  # after a rebuild, there are no db-loc-lock flags
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --noloclockchk --debug ${DBG} $DATADB $TDBRDT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc
fi

if [[ $TESTON == T ]]; then
  # test secondary folder: regex db : get dance/title from file path
  tname=rebuild-file-path-dt-second
  tdir="$(dirname ${musicdir})"
  setorgpath '{%DANCE%/}{%TITLE%}'
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
  exitonfail $rc $crc
fi

if [[ $TESTON == T ]]; then
  # test regex db : get date/tracknum-artist-title from file path
  tname=rebuild-file-path-dtat
  setorgpath '{%DANCE%/}{%TRACKNUMBER0%-}{%ARTIST% - }{%TITLE%}'
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --rebuild \
      --dbupmusicdir "${musicdir}" \
      --cli --wait --verbose)
  exp="found ${NUMREGEX} skip 0 indb 0 new ${NUMREGEX} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  # after a rebuild, there are no db-loc-lock flags
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --noloclockchk --debug ${DBG} $DATADB $TDBRDTAT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc
fi

if [[ $TESTON == T ]]; then
  test -d "$SECONDMUSICDIR" || mkdir -p "$SECONDMUSICDIR"

  # re-create both the main and second music dir

  ./src/utils/mktestsetup.sh --force --debug ${DBG} ${ATIFLAG}

  ./src/utils/mktestsetup.sh \
      --infile $INSECOND \
      --outfile $TDBSECOND \
      --debug ${DBG} ${ATIFLAG} \
      --dbupmusicdir "$SECONDMUSICDIR" \
      --nodbcopy \
      --keepdb

  # second test db : check-new with a second dir
  tname=second-checknew
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${SECONDMUSICDIR}" \
      --cli --wait --verbose)

  exp="found ${NUM2} skip 0 indb 0 new ${NUM2} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBMAIN $TDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc

  # keep a copy of the combined main+second database.
  cp -f $DATADB $KDBSECOND
fi

if [[ $TESTON == T ]]; then
  # main+second test db : compact with no changes
  # compact must iterate through the database, not just
  # the specified directory.
  tname=second-compact-basic
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --compact \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated ${NUM2TOT} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc
fi

if [[ $TESTON == T ]]; then
  # clean any leftovers from the tmp dir
  rm -rf $TMPDIRA $TMPDIRB
  mkdir $TMPDIRA
  mkdir $TMPDIRB
  # save all foxtrot
  mv -f $musicdir/*-foxtrot.mp3 $TMPDIRA
  mv -f "$SECONDMUSICDIR"/*-foxtrot.mp3 $TMPDIRB

  ./src/utils/mktestsetup.sh \
      --infile $INSECONDNOFT \
      --outfile $TDBSECONDNOFT \
      --dbupmusicdir "$SECONDMUSICDIR" \
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
  trc=$?
  rc=$trc
  updateCounts $rc

  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${SECONDMUSICDIR}" \
      --cli --wait --verbose)
  exp="found ${NUM2_NOFT} skip ${NUM2_NOFT} indb ${NUM2_NOFT} new 0 updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  trc=$?
  if [[ $trc -ne 0 ]]; then
    rc=$trc
  fi
  updateCounts $rc
  # no db comparison
  dispres $tname $rc
  exitonfail $rc
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
  exp="found ${NUM2TOT_NOFT} skip 0 indb ${NUM2TOT_NOFT} new 0 updated ${NUM2TOT_NOFT} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $TDBNOFOXTROT $TDBSECONDNOFT)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $rc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc

  # restore all foxtrot
  mv -f $TMPDIRA/* $musicdir
  mv -f $TMPDIRB/* "$SECONDMUSICDIR"

  # restore the full main+second database
  cp -f $KDBSECOND $DATADB
fi

if [[ $TESTON == T ]]; then
  # clean all of the tags from the music files
  cleanallaudiofiletags $musicdir
  cleanallaudiofiletags "$SECONDMUSICDIR"

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
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated 0 renamed 0 norename 0 notaudio 0 writetag ${NUM2TOT}"
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
  exitonfail $rc $crc
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
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated ${NUM2TOT} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc
  # an empty db has no db-loc-lock flags
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --noloclockchk --debug ${DBG} $DATADB $KDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"
  dispres $tname $rc $crc
  exitonfail $rc $crc
fi

if [[ $TESTON == T ]]; then
  # restore the main+second database
  cp -f $KDBSECOND $DATADB

  setorgpath '{%DANCE%/}{%TITLE%}'

  # main+second : re-org with 'dance/title'
  tname=second-reorg-basic-dir
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated 0 renamed ${NUM2TOT_RN} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc

  reorgrc=0

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Announce"
  for fn in ann-samba.mp3 ann-waltz.mp3 ann-tango.mp3 ann-foxtrot.mp3 ann-quickstep.mp3; do
    checkreorg ann "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  dispres $tname $rc $reorgrc
  exitonfail $rc $reorgrc
fi

if [[ $TESTON == T ]]; then
  setorgpath '{%DANCE% - }{%TITLE%}'

  # main+second : re-org with 'dance - title'
  tname=second-reorg-basic-dash
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated 0 renamed ${NUM2TOT_RN} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc

  reorgrc=0

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Announce"
  for fn in ann-samba.mp3 ann-waltz.mp3 ann-tango.mp3 ann-foxtrot.mp3 ann-quickstep.mp3; do
    checkreorg ann "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg dash "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg dash "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  if [[ -d "${tmdir}/${dance}" ]]; then
    echo "ERR: ${tmdir}/${dance} not removed"
    reorgrc=1
  fi
  if [[ -d "${omdir}/${dance}" ]]; then
    echo "ERR: ${tmdir}/${dance} not removed"
    reorgrc=1
  fi

  dispres $tname $rc $reorgrc
  exitonfail $rc $reorgrc
fi

if [[ $TESTON == T ]]; then
  setorgpath '{%TITLE%}'

  # main+second : re-org title only
  tname=second-reorg-basic-title
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated 0 renamed ${NUM2TOT_RN} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc

  reorgrc=0

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Announce"
  for fn in ann-samba.mp3 ann-waltz.mp3 ann-tango.mp3 ann-foxtrot.mp3 ann-quickstep.mp3; do
    checkreorg ann "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  for dance in "Cha Cha" Jive Quickstep; do
    if [[ -d "${tmdir}/${dance}" ]]; then
      echo "ERR: ${tmdir}/${dance} not removed"
      reorgrc=1
    fi
    if [[ -d "${omdir}/${dance}" ]]; then
      echo "ERR: ${tmdir}/${dance} not removed"
      reorgrc=1
    fi
  done

  # the important test for re-organize
  # must match up against the original database.
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"

  dispres $tname $rc $reorgrc $crc
  exitonfail $rc $reorgrc $crc
fi

if [[ $TESTON == T ]]; then
  # create some .original files
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  createoriginal "$tmdir" 003-jive.mp3
  createoriginal "$omdir" 001-alt-jive.mp3

  setorgpath '{%DANCE%/}{%TITLE%}'

  # main+second : re-org test with .original
  tname=second-reorg-original
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated 0 renamed ${NUM2TOT_RN} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc

  reorgrc=0

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Announce"
  for fn in ann-samba.mp3 ann-waltz.mp3 ann-tango.mp3 ann-foxtrot.mp3 ann-quickstep.mp3; do
    checkreorg ann "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Jive"
  for fn in 003-jive.mp3 003-jive.mp3.original; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Jive"
  for fn in 001-alt-jive.mp3 001-alt-jive.mp3.original; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  dispres $tname $rc $reorgrc
  exitonfail $rc $reorgrc
fi

if [[ $TESTON == T ]]; then
  setorgpath '{%TITLE%}'

  # main+second : re-org test with .original
  tname=second-reorg-original-title
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated 0 renamed ${NUM2TOT_RN} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc

  reorgrc=0

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Announce"
  for fn in ann-samba.mp3 ann-waltz.mp3 ann-tango.mp3 ann-foxtrot.mp3 ann-quickstep.mp3; do
    checkreorg ann "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Jive"
  for fn in 003-jive.mp3 003-jive.mp3.original; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Jive"
  for fn in 001-alt-jive.mp3 001-alt-jive.mp3.original; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  for dance in "Cha Cha" Jive Quickstep; do
    if [[ -d "${tmdir}/${dance}" ]]; then
      echo "ERR: ${tmdir}/${dance} not removed"
      reorgrc=1
    fi
    if [[ -d "${omdir}/${dance}" ]]; then
      echo "ERR: ${tmdir}/${dance} not removed"
      reorgrc=1
    fi
  done

  # the important test for re-organize
  # must match up against the original database.
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"

  dispres $tname $rc $reorgrc $crc
  exitonfail $rc $reorgrc $crc
fi

if [[ $TESTON == T ]]; then
  # create some existing files
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Quickstep"
  createexisting "$tmdir" "$dance" 002-quickstep.mp3
  createexisting "$omdir" "$dance" 001-alt-quickstep.mp3

  setorgpath '{%DANCE%/}{%TITLE%}'

  # main+second : re-org test with existing files
  tname=second-reorg-existing
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated 0 renamed ${NUM2TOT_RN_EXIST} norename ${NUM2_EXIST} notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc

  reorgrc=0

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Announce"
  for fn in ann-samba.mp3 ann-waltz.mp3 ann-tango.mp3 ann-foxtrot.mp3 ann-quickstep.mp3; do
    checkreorg ann "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Jive"
  for fn in 003-jive.mp3 003-jive.mp3.original; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  # the quickstep should still exist w/o being renamed
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Quickstep"
  for fn in 002-quickstep.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Jive"
  for fn in 001-alt-jive.mp3 001-alt-jive.mp3.original; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  # the quickstep should still exist w/o being renamed
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Quickstep"
  for fn in 001-alt-quickstep.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  dispres $tname $rc $reorgrc
  exitonfail $rc $reorgrc
fi

if [[ $TESTON == T ]]; then
  # remove the existing files and run the re-org again.
  tmdir=${cwd}/test-music
  rm -f "${tmdir}/Quickstep/002-quickstep.mp3"
  tmdir=${cwd}/tmp/music-second
  rm -f "${tmdir}/Quickstep/001-alt-quickstep.mp3"

  setorgpath '{%DANCE%/}{%TITLE%}'

  # main+second : re-org test with existing files removed
  tname=second-reorg-exist-clean
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated 0 renamed ${NUM2_EXIST} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc

  reorgrc=0

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Announce"
  for fn in ann-samba.mp3 ann-waltz.mp3 ann-tango.mp3 ann-foxtrot.mp3 ann-quickstep.mp3; do
    checkreorg ann "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Jive"
  for fn in 003-jive.mp3 003-jive.mp3.original; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Quickstep"
  for fn in 002-quickstep.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Jive"
  for fn in 001-alt-jive.mp3 001-alt-jive.mp3.original; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Quickstep"
  for fn in 001-alt-quickstep.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  dispres $tname $rc $reorgrc
  exitonfail $rc $reorgrc
fi

if [[ $TESTON == T ]]; then
  # and back to the title version again to make sure everything is ok.

  setorgpath '{%TITLE%}'

  # main+second : re-org test, existing files removed, back to title
  tname=second-reorg-exist-clean-title
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated 0 renamed ${NUM2TOT_RN} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  rc=$?
  updateCounts $rc

  reorgrc=0

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Announce"
  for fn in ann-samba.mp3 ann-waltz.mp3 ann-tango.mp3 ann-foxtrot.mp3 ann-quickstep.mp3; do
    checkreorg ann "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Jive"
  for fn in 003-jive.mp3 003-jive.mp3.original; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Jive"
  for fn in 001-alt-jive.mp3 001-alt-jive.mp3.original; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  for dance in "Cha Cha" Jive Quickstep; do
    if [[ -d "${tmdir}/${dance}" ]]; then
      echo "ERR: ${tmdir}/${dance} not removed"
      reorgrc=1
    fi
    if [[ -d "${omdir}/${dance}" ]]; then
      echo "ERR: ${tmdir}/${dance} not removed"
      reorgrc=1
    fi
  done

  # the important test for re-organize
  # must match up against the original database.
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"

  dispres $tname $rc $reorgrc $crc
  exitonfail $rc $reorgrc $crc
fi

if [[ $TESTON == T ]]; then
  # set orgpath to dance/title
  setorgpath '{%DANCE%/}{%TITLE}'

  # clean up the originals.  these will mess up the check-new counts.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  removeoriginal "$tmdir" 003-jive.mp3
  removeoriginal "$omdir" 001-alt-jive.mp3

  # main+second : test auto-reorg
  tname=auto-reorg-checknew-dir

  # clean any leftovers from the tmp dir
  rm -rf $TMPDIRA $TMPDIRB
  mkdir $TMPDIRA
  mkdir $TMPDIRB
  # save all foxtrot
  mv -f $musicdir/*-foxtrot.mp3 $TMPDIRA
  mv -f "$SECONDMUSICDIR"/*-foxtrot.mp3 $TMPDIRB

  # restore the main+second database
  cp -f $KDBSECOND $DATADB

  # remove the foxtrots from the db.
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --compact \
      --cli --wait --verbose)
  exp="found ${NUM2TOT_NOFT} skip 0 indb ${NUM2TOT_NOFT} new 0 updated ${NUM2TOT_NOFT} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  trc=$?
  rc=$trc
  updateCounts $rc

  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUM2TOT_NOFT} skip 0 indb ${NUM2TOT_NOFT} new 0 updated 0 renamed ${NUM2TOT_RN_NOFT} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  trc=$?
  if [[ $trc -ne 0 ]]; then
    rc=$trc
  fi
  updateCounts $trc

  # save this database after the re-org, no foxtrots
  cp -f $DATADB $KDBREORGNOFT

  # restore all foxtrot
  mv -f $TMPDIRA/* $musicdir
  mv -f $TMPDIRB/* "$SECONDMUSICDIR"

  # main+second : run a check-new before without auto-reorg on
  # there should be no renames
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --cli --wait --verbose)
  exp="found ${NUMNORM} skip ${NUMNOFT} indb ${NUMNOFT} new ${NUMFT} updated 0 renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  trc=$?
  if [[ $trc -ne 0 ]]; then
    rc=$trc
  fi
  updateCounts $trc

  reorgrc=0

  # the foxtrots should still be named by title.
  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Foxtrot"
  for fn in 005-foxtrot.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Foxtrot"
  for fn in 001-alt-foxtrot.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # restore the saved database without foxtrot
  cp -f $KDBREORGNOFT $DATADB

  # turn auto-org on
  setautoorgon

  # main+second : run a check-new again for the main dir
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --cli --wait --verbose)
  exp="found ${NUMNORM} skip ${NUMNOFT} indb ${NUMNOFT} new ${NUMFT} updated 0 renamed ${NUMFT} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  trc=$?
  if [[ $trc -ne 0 ]]; then
    rc=$trc
  fi
  updateCounts $trc

  # main+second : run a check-new for the second dir
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --checknew \
      --dbupmusicdir "${SECONDMUSICDIR}" \
      --cli --wait --verbose)
  exp="found ${NUM2} skip ${NUM2_NOFT} indb ${NUM2_NOFT} new ${NUM2_FT} updated 0 renamed ${NUM2_FT} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  trc=$?
  if [[ $trc -ne 0 ]]; then
    rc=$trc
  fi
  updateCounts $trc

  # run another compact just to check the numbers
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --compact \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated ${NUM2TOT} renamed 0 norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  trc=$?
  if [[ $trc -ne 0 ]]; then
    rc=$trc
  fi
  updateCounts $trc

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Announce"
  for fn in ann-samba.mp3 ann-waltz.mp3 ann-tango.mp3 ann-foxtrot.mp3 ann-quickstep.mp3; do
    checkreorg ann "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Jive"
  for fn in 003-jive.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # the foxtrots should now be named by dance/title
  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Foxtrot"
  for fn in 005-foxtrot.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Foxtrot"
  for fn in 001-alt-foxtrot.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Jive"
  for fn in 001-alt-jive.mp3; do
    checkreorg dir "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  dispres $tname $rc $reorgrc
  exitonfail $rc $reorgrc
fi

if [[ $TESTON == T ]]; then
  # and back to the title version again to make sure everything is ok.

  setorgpath '{%TITLE%}'

  rc=0
  tname=reorg-title-check

  # main+second : re-org by title
  got=$(./bin/bdj4 --bdj4dbupdate \
      --debug ${DBG} \
      --reorganize \
      --cli --wait --verbose)
  exp="found ${NUM2TOT} skip 0 indb ${NUM2TOT} new 0 updated 0 renamed ${NUM2TOT_RN} norename 0 notaudio 0 writetag 0"
  msg+=$(checkres $tname "$got" "$exp")
  trc=$?
  if [[ $trc -ne 0 ]]; then
    rc=$trc
  fi
  updateCounts $trc

  reorgrc=0

  # music-dir announcements
  # announcements should be locked and stay where they are.
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Announce"
  for fn in ann-samba.mp3 ann-waltz.mp3 ann-tango.mp3 ann-foxtrot.mp3 ann-quickstep.mp3; do
    checkreorg ann "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Cha Cha"
  for fn in 006-chacha.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Jive"
  for fn in 003-jive.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # music-dir
  tmdir=${cwd}/test-music
  omdir=${cwd}/tmp/music-second
  dance="Foxtrot"
  for fn in 005-foxtrot.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Foxtrot"
  for fn in 001-alt-foxtrot.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Cha Cha"
  for fn in 001-alt-chacha.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  # secondary
  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  dance="Jive"
  for fn in 001-alt-jive.mp3; do
    checkreorg title "$tmdir" "$omdir" "$dance" "$fn"
    trc=$?
    if [[ $trc -ne 0 ]]; then
      reorgrc=1
    fi
  done

  tmdir=${cwd}/tmp/music-second
  omdir=${cwd}/test-music
  for dance in "Cha Cha" Jive Quickstep Foxtrot; do
    if [[ -d "${tmdir}/${dance}" ]]; then
      echo "ERR: ${tmdir}/${dance} not removed"
      reorgrc=1
    fi
    if [[ -d "${omdir}/${dance}" ]]; then
      echo "ERR: ${tmdir}/${dance} not removed"
      reorgrc=1
    fi
  done

  # the important test for re-organize
  # must match up against the original database.
  msg+="$(./bin/bdj4 --tdbcompare ${VERBOSE} --debug ${DBG} $DATADB $KDBSECOND)"
  crc=$?
  updateCounts $crc
  msg+="$(compcheck $tname $crc)"

  setautoorgoff

  dispres $tname $rc $reorgrc $crc
  exitonfail $rc $reorgrc $crc
fi

setwritetagsoff
setautoorgoff

# remove test db, temporary files
rm -f $INCOMPAT $KDBMAIN
rm -f $TDBNOCHACHA $TDBCHACHA $TDBEMPTY $TDBCOMPACT $TDBCOMPAT $TDBNOFOXTROT
rm -f $TDBRDAT $TDBRDT $TDBRDTSECOND $TDBRDTAT
rm -f $TDBSECOND $KDBSECOND $TDBSECONDNOFT $TDBSECONDEMPTY $KDBREORGNOFT
rm -f $TMPA $TMPB
rm -rf $TMPDIRDT "$SECONDMUSICDIR" $TMPDIRA $TMPDIRB

echo "tests: $tcount pass: $pass fail: $fail"
rc=1
if [[ $tcount -eq $pass ]]; then
  echo "==final OK"
  rc=0
else
  echo "==final FAIL"
fi

exit $rc
