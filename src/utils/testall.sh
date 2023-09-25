#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

grc=0

echo "-- $(date +%T) building"
(
  cd src
  make distclean
)
LOG=src/testall.log
> $LOG
(
  cd src
  make
) >> $LOG 2>&1

echo "-- $(date +%T) warnings"
# windows has a multitude of warnings in check.h
grep warning $LOG |
    grep -v 'ignoring duplicate libraries' |
    grep -v 'check\.h' |
    grep -v 'mongoose\.c' |
    grep -v 'warning generated'

echo "-- check" >> $LOG
echo "-- $(date +%T) make test setup"
./src/utils/mktestsetup.sh --force >> $LOG 2>&1
echo "-- $(date +%T) check"
./bin/bdj4 --check_all >> $LOG 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "-- $(date +%T) check FAIL"
  grc=1
else
  echo "-- $(date +%T) check OK"
fi

if [[ $grc -eq 0 ]]; then
  echo "-- dbtest mutagen" >> $LOG
  echo "-- $(date +%T) dbtest mutagen"
  # dbtest will rebuild the databases.
  ./src/utils/dbtest.sh --atimutagen >> $LOG 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "-- $(date +%T) dbtest mutagen FAIL"
    grc=1
  else
    echo "-- $(date +%T) dbtest mutagen OK"
  fi

  # dbtest will rebuild the databases.
  echo "-- dbtest atibdj4" >> $LOG
  echo "-- $(date +%T) dbtest atibdj4"
  ./src/utils/dbtest.sh --atibdj4 >> $LOG 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "-- $(date +%T) dbtest atibdj4 FAIL"
    grc=1
  else
    echo "-- $(date +%T) dbtest atibdj4 OK"
  fi
else
  echo "dbtest not run"
fi

if [[ $grc -eq 0 ]]; then
  echo "-- insttest" >> $LOG
  echo "-- $(date +%T) insttest"
  # insttest will rebuild the databases.
  ./src/utils/insttest.sh >> $LOG 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "-- $(date +%T) insttest FAIL"
    grc=1
  else
    echo "-- $(date +%T) insttest OK"
  fi
fi

if [[ $grc -eq 0 ]]; then
  echo "-- testsuite" >> $LOG
  echo "-- $(date +%T) make test setup"
  ./src/utils/mktestsetup.sh --force >> $LOG 2>&1
  echo "-- $(date +%T) testsuite"
  ./bin/bdj4 --testsuite >> $LOG 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "-- $(date +%T) testsuite FAIL"
    grc=1
  else
    echo "-- $(date +%T) testsuite OK"
  fi
else
  echo "testsuite not run"
fi

if [[ $grc -eq 0 ]]; then
  echo "-- $(date +%T) creating package"
  ./src/utils/repkg.sh
  echo "-- $(date +%T) finish OK"
else
  echo "-- $(date +%T) finish FAIL"
fi

echo "LOG: $LOG"
exit $grc
