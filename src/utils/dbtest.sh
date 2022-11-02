#!/bin/bash

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
    echo "    Test: $tname"
    echo "     Got: $r"
    echo "Expected: $e"
    fail=$(($fail+1))
  fi
}

# get music dir
hostname=$(hostname)
mconf=../data/${hostname}/bdjconfig.txt
musicdir=$(sed -n -e '/^DIRMUSIC/ { n; s/^\.\.//; p }' $mconf)

# rebuild of standard test database
got=$(../bin/bdj4 --bdj4dbupdate \
  --rebuild \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found 140 skip 0 indb 0 new 140 updated 0 notaudio 0"
check rebuild-basic "$got" "$exp"

# check-new with no changes
got=$(../bin/bdj4 --bdj4dbupdate \
  --checknew \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found 140 skip 140 indb 140 new 0 updated 0 notaudio 0"
check checknew-basic "$got" "$exp"

# update-from-tags with no changes
got=$(../bin/bdj4 --bdj4dbupdate \
  --updfromtags \
  --dbtopdir "${musicdir}" \
  --cli --wait --verbose)
exp="found 140 skip 0 indb 140 new 0 updated 140 notaudio 0"
check updfromtags-basic "$got" "$exp"

echo "tests: $tcount pass: $pass fail: $fail"
rc=1
if [[ $tcount -eq $pass ]]; then
  echo OK
  rc=0
else
  echo FAIL
fi

exit $rc
