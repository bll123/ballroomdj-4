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

tcount=0
pass=0
fail=0

function check {
  tname=$1
  r=$2
  e=$3
  tcount=$(($tcount+1))
  if [[ $r == $e ]]; then
    pass=$(($pass+1))
    trc=0
  else
    echo "    FAIL: $tname"
    echo "     Got: $r"
    echo "Expected: $e"
    fail=$(($fail+1))
    trc=1
  fi
  return $trc
}

function compcheck {
  tname=$1
  retc=$2
  tcount=$(($tcount+1))
  if [[ $retc -eq 0 ]]; then
    pass=$(($pass+1))
    trc=0
  else
    echo "    FAIL: $tname (db compare)"
    fail=$(($fail+1))
    trc=1
  fi
  return $trc
}

function checkaudiotags {
  grc=0
  for f in test-music/*; do
    ./bin/bdj4 --msys --bdj4tags "$f" > $TMPA
    ./bin/bdj4 --msys --tdbdump data/musicdb.dat "$(basename $f)" > $TMPB
    diff $TMPA $TMPB > /dev/null 2>&1
    trc=$?
    if [[ $trc -ne 0 ]]; then
      grc=1
    fi
  done
  return $grc
}

function disp {
  tname=$1
  rca=$2
  rcb=$3

  if [[ $rca -eq 0 && $rcb -eq 0 ]]; then
    echo "$tname OK"
  else
    echo "$tname FAIL"
  fi
}

function setwritetagson {
  gconf=data/bdjconfig.txt
  sed -e '/^WRITETAGS/ { n ; s/.*/..ALL/ ; }' \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function setbdj3compaton {
  gconf=data/bdjconfig.txt
  sed -e '/^BDJ3COMPATTAGS/ { n ; s/.*/..yes/ ; }' \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function setbdj3compatoff {
  gconf=data/bdjconfig.txt
  sed -e '/^BDJ3COMPATTAGS/ { n ; s/.*/..no/ ; }' \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

function cleanallaudiofiletags {
  for f in test-music/*; do
    ./bin/bdj4 --msys --bdj4tags --cleantags --quiet "$f"
  done
}

function setorgregex {
  org=$1

  gconf=data/bdjconfig.txt
  sed -e "/^ORGPATH/ { n ; s~.*~..${org}~ ; }" \
      ${gconf} > ${gconf}.n
  mv -f ${gconf}.n ${gconf}
}

NUMM=119
NUMB=15
NUMBL1=$(($NUMB-1))
NUMR=13

INB=test-templates/test-m-b.txt
INC=test-templates/test-m-c.txt
IND=test-templates/test-m-c.txt
INR=test-templates/test-m-regex.txt
INRDAT=test-templates/test-m-regex-dat.txt
INRDT=test-templates/test-m-regex-dt.txt
INRDTAT=test-templates/test-m-regex-dtat.txt
TDBB=tmp/test-m-b.dat
TDBC=tmp/test-m-c.dat
TDBD=tmp/test-m-d.dat
TDBRDAT=tmp/test-m-r-dat.dat
TDBRDT=tmp/test-m-r-dt.dat
TDBRDTAT=tmp/test-m-r-dtat.dat
TMPA=tmp/dbtesta.txt
TMPB=tmp/dbtestb.txt

./src/utils/mktestsetup.sh --force

# get music dir
hostname=$(hostname)
mconf=data/${hostname}/bdjconfig.txt
musicdir=$(sed -n -e '/^DIRMUSIC/ { n; s/^\.\.//; p ; }' $mconf)

# main test db : rebuild of standard test database
tname=rebuild-basic
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --rebuild \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMM} skip 0 indb 0 new ${NUMM} updated 0 notaudio 0 writetag 0"
check $tname "$got" "$exp"
rc=$?
./bin/bdj4 --msys --tdbcompare data/musicdb.dat test-templates/musicdb.dat
crc=$?
compcheck $tname $rc
disp $tname $rc $crc

# main test db : check-new with no changes
tname=checknew-basic
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --checknew \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMM} skip ${NUMM} indb ${NUMM} new 0 updated 0 notaudio 0 writetag 0"
check $tname "$got" "$exp"
rc=$?
./bin/bdj4 --msys --tdbcompare data/musicdb.dat test-templates/musicdb.dat
crc=$?
compcheck $tname $rc
disp $tname $rc $crc

# main test db : update-from-tags with no changes
tname=updfromtags-basic
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --updfromtags \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMM} skip 0 indb ${NUMM} new 0 updated ${NUMM} notaudio 0 writetag 0"
check $tname "$got" "$exp"
rc=$?
./bin/bdj4 --msys --tdbcompare data/musicdb.dat test-templates/musicdb.dat
crc=$?
compcheck $tname $rc
disp $tname $rc $crc

# create test db w/no data
./src/utils/mktestsetup.sh \
    --emptydb \
    --infile $INC \
    --outfile $TDBD
# create test db w/all songs
./src/utils/mktestsetup.sh \
    --infile $INC \
    --outfile $TDBC
# save the cha cha
mv -f test-music/001-chacha.mp3 tmp
# create test db w/o chacha
# will copy tmp/test-m-b.dat to data/
./src/utils/mktestsetup.sh \
    --infile $INB \
    --outfile $TDBB

# test db : rebuild of test-m-b
if [[ -f test-music/001-chacha.mp3 ]]; then
  echo "cha cha present when it should not be"
  rc=1
  crc=0
  disp $tname $rc $crc
else
  tname=rebuild-test-db
  got=$(./bin/bdj4 --msys --bdj4dbupdate \
    --debug 262175 \
    --rebuild \
    --dbtopdir "${musicdir}" \
    --cli --wait --verbose)
  exp="found ${NUMBL1} skip 0 indb 0 new ${NUMBL1} updated 0 notaudio 0 writetag 0"
  check $tname "$got" "$exp"
  rc=$?
  ./bin/bdj4 --msys --tdbcompare data/musicdb.dat $TDBB
  crc=$?
  compcheck $tname $rc
  disp $tname $rc $crc
fi

# restore the cha cha
mv -f tmp/001-chacha.mp3 test-music

# test db : check-new w/cha cha
tname=checknew-chacha
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --checknew \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMB} skip ${NUMBL1} indb ${NUMBL1} new 1 updated 0 notaudio 0 writetag 0"
check $tname "$got" "$exp"
rc=$?
./bin/bdj4 --msys --tdbcompare data/musicdb.dat $TDBC
crc=$?
compcheck $tname $crc
disp $tname $rc $crc

# clean all of the tags from the music files
cleanallaudiofiletags

# test db : rebuild with no tags
tname=rebuild-no-tags
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --rebuild \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMB} skip 0 indb 0 new ${NUMB} updated 0 notaudio 0 writetag 0"
check $tname "$got" "$exp"
rc=$?
# no db comparison
disp $tname $rc $rc

# restore the -c database, needed for write tags check
cp -f $TDBC data/musicdb.dat

# test db : write tags
tname=writetags-bdj3-compat-on
setwritetagson
setbdj3compaton
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --writetags \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMB} skip 0 indb ${NUMB} new 0 updated 0 notaudio 0 writetag ${NUMB}"
check $tname "$got" "$exp"
rc=$?
./bin/bdj4 --msys --tdbcompare data/musicdb.dat $TDBC
crc=$?
compcheck $tname $crc

if [[ $rc -eq 0 && $crc -eq 0 ]]; then
  checkaudiotags
  trc=$?
  if [[ $trc -ne 0 ]]; then
    rc=$trc
  fi
fi
disp $tname $rc $crc

# restore the -c database again, needed for write tags check
cp -f $TDBC data/musicdb.dat

# test db : write tags
tname=writetags-bdj3-compat-off
setwritetagson
setbdj3compatoff
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --writetags \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMB} skip 0 indb ${NUMB} new 0 updated 0 notaudio 0 writetag ${NUMB}"
check $tname "$got" "$exp"
rc=$?
./bin/bdj4 --msys --tdbcompare data/musicdb.dat $TDBC
crc=$?
compcheck $tname $crc

if [[ $rc -eq 0 && $crc -eq 0 ]]; then
  checkaudiotags
  trc=$?
  if [[ $trc -ne 0 ]]; then
    rc=$trc
  fi
fi
disp $tname $rc $crc

# restore the -d database (empty of tags), needed for update from tags check
cp -f $TDBD data/musicdb.dat

# test db : update from tags
tname=update-from-tags-empty-db
setwritetagson
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --updfromtags \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMB} skip 0 indb ${NUMB} new 0 updated ${NUMB} notaudio 0 writetag 0"
check $tname "$got" "$exp"
rc=$?
./bin/bdj4 --msys --tdbcompare data/musicdb.dat $TDBC
crc=$?
compcheck $tname $crc

if [[ $rc -eq 0 && $crc -eq 0 ]]; then
  checkaudiotags
  trc=$?
  if [[ $trc -ne 0 ]]; then
    rc=$trc
  fi
fi
disp $tname $rc $crc

# create test regex db w/tags (dance/artist - title)
./src/utils/mktestsetup.sh \
    --infile $INRDAT \
    --outfile $TDBRDAT
# create test regex db w/tags (dance/title)
./src/utils/mktestsetup.sh \
    --infile $INRDT \
    --outfile $TDBRDT
# create test regex db w/tags (dance/tn-artist - title)
./src/utils/mktestsetup.sh \
    --infile $INRDTAT \
    --outfile $TDBRDTAT
# create test regex db w/o tags
# only want test music, not db
./src/utils/mktestsetup.sh \
    --infile $INR \
    --outfile $TMPA

# test regex db : get artist/title from file path
tname=rebuild-file-path-dat
setorgregex '{%DANCE%/}{%ARTIST% - }{%TITLE%}'
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --rebuild \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMR} skip 0 indb 0 new ${NUMR} updated 0 notaudio 0 writetag 0"
check $tname "$got" "$exp"
rc=$?
./bin/bdj4 --msys --tdbcompare data/musicdb.dat $TDBRDAT
crc=$?
compcheck $tname $crc
disp $tname $rc $crc

# test regex db : get artist/title from file path
tname=rebuild-file-path-dt
setorgregex '{%DANCE%/}{%TITLE%}'
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --rebuild \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMR} skip 0 indb 0 new ${NUMR} updated 0 notaudio 0 writetag 0"
check $tname "$got" "$exp"
rc=$?
./bin/bdj4 --msys --tdbcompare data/musicdb.dat $TDBRDT
crc=$?
compcheck $tname $crc
disp $tname $rc $crc

# test regex db : get tracknum-artist/title from file path
tname=rebuild-file-path-dtat
setorgregex '{%DANCE%/}{%TRACKNUMBER0%-}{%ARTIST% - }{%TITLE%}'
got=$(./bin/bdj4 --msys --bdj4dbupdate \
  --debug 262175 \
  --rebuild \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found ${NUMR} skip 0 indb 0 new ${NUMR} updated 0 notaudio 0 writetag 0"
check $tname "$got" "$exp"
rc=$?
./bin/bdj4 --msys --tdbcompare data/musicdb.dat $TDBRDTAT
crc=$?
compcheck $tname $crc
disp $tname $rc $crc

# remove test db, temporary files
rm -f $TDBB $TDBC $TDBD TDBRDAT TDBRDT TDBRDTAT $TMPA $TMPB

echo "tests: $tcount pass: $pass fail: $fail"
rc=1
if [[ $tcount -eq $pass ]]; then
  echo OK
  rc=0
else
  echo FAIL
fi


exit $rc
