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
  else
    echo "    FAIL: $tname"
    echo "     Got: $r"
    echo "Expected: $e"
    fail=$(($fail+1))
  fi
}

function compcheck {
  tname=$1
  retc=$2
  tcount=$(($tcount+1))
  if [[ $retc -eq 0 ]]; then
    pass=$(($pass+1))
  else
    echo "    FAIL: $tname (db compare)"
    fail=$(($fail+1))
  fi
}

# get music dir
hostname=$(hostname)
mconf=data/${hostname}/bdjconfig.txt
musicdir=$(sed -n -e '/^DIRMUSIC/ { n; s/^\.\.//; p }' $mconf)

./src/utils/mktestsetup.sh --force

# main test db : rebuild of standard test database
tname=rebuild-basic
got=$(./bin/bdj4 --bdj4dbupdate \
  --rebuild \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found 119 skip 0 indb 0 new 119 updated 0 notaudio 0"
check $tname "$got" "$exp"
./bin/bdj4 --tdbcompare data/musicdb.dat test-templates/musicdb.dat
rc=$?
compcheck $tname $rc

# main test db : check-new with no changes
tname=checknew-basic
got=$(./bin/bdj4 --bdj4dbupdate \
  --checknew \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found 119 skip 119 indb 119 new 0 updated 0 notaudio 0"
check $tname "$got" "$exp"
./bin/bdj4 --tdbcompare data/musicdb.dat test-templates/musicdb.dat
rc=$?
compcheck $tname $rc

# main test db : update-from-tags with no changes
tname=updfromtags-basic
got=$(./bin/bdj4 --bdj4dbupdate \
  --updfromtags \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found 119 skip 0 indb 119 new 0 updated 119 notaudio 0"
check $tname "$got" "$exp"
./bin/bdj4 --tdbcompare data/musicdb.dat test-templates/musicdb.dat
rc=$?
compcheck $tname $rc

TDBB=tmp/test-m-b.dat
TDBC=tmp/test-m-c.dat
./src/utils/mktestsetup.sh \
    --infile test-templates/test-m-c.txt \
    --outfile $TDBC
# save the cha cha
mv -f test-music/001-chacha.mp3 tmp
# will copy tmp/test-m-b.dat to data/
./src/utils/mktestsetup.sh \
    --infile test-templates/test-m-b.txt \
    --outfile $TDBB

# test db : rebuild of test-m-b
tname=rebuild-b
got=$(./bin/bdj4 --bdj4dbupdate \
  --rebuild \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found 10 skip 0 indb 0 new 10 updated 0 notaudio 0"
check $tname "$got" "$exp"
./bin/bdj4 --tdbcompare data/musicdb.dat $TDBB
rc=$?
compcheck $tname $rc

# restore the cha cha
mv -f tmp/001-chacha.mp3 test-music

# test db : check-new w/cha cha
tname=checknew-chacha
got=$(./bin/bdj4 --bdj4dbupdate \
  --checknew \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found 11 skip 10 indb 10 new 1 updated 0 notaudio 0"
check $tname "$got" "$exp"
./bin/bdj4 --tdbcompare data/musicdb.dat $TDBC
rc=$?
compcheck $tname $rc

# remove test db
rm -f $TDBB $TDBC

echo "tests: $tcount pass: $pass fail: $fail"
rc=1
if [[ $tcount -eq $pass ]]; then
  echo OK
  rc=0
else
  echo FAIL
fi

exit $rc
