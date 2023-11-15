#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

grc=0

TBUILD=T
TCHECK=T
DBTEST=T
INSTTEST=T
TESTSUITE=T
for arg in "$@"; do
  case $arg in
    --nobuild)
      TBUILD=F
      ;;
    --notest)
      TCHECK=F
      DBTEST=F
      INSTTEST=F
      TESTSUITE=F
      ;;
    --testsuite)
      TBUILD=F
      TCHECK=F
      DBTEST=F
      INSTTEST=F
      ;;
  esac
done

. ./src/utils/pkgnm.sh
pkgnmgetdata

if [[ $TBUILD == T ]]; then
  echo "-- $(date +%T) building"
  (
    cd src
    make distclean
  )
fi

LOG=src/testall.log
> $LOG

if [[ $TBUILD == T ]]; then
  (
    cd src
    case ${pn_dist} in
      -opensuse)
        make CC=gcc-12 CXX=g++-12
        ;;
      *)
        make
        ;;
    esac
  ) >> $LOG 2>&1

  echo "-- $(date +%T) warnings"
  # windows has a multitude of warnings in check.h
  grep warning $LOG |
      grep -v 'ignoring duplicate libraries' |
      grep -v 'check\.h' |
      grep -v 'mongoose\.c' |
      grep -v 'warning generated'
fi

if [[ $TBUILD == T && $pn_tag == win64 ]]; then
  # for windows, make sure the libraries in plocal are up to date
  ./pkg/prepkg.sh
fi

if [[ $TCHECK == T ]]; then
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
fi

if [[ $DBTEST == T ]]; then
  if [[ $grc -eq 0 ]]; then
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
fi

if [[ $INSTTEST == T ]]; then
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
fi

if [[ $TESTSUITE == T ]]; then
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
fi

if [[ $grc -eq 0 ]]; then
  echo "-- $(date +%T) running prepkg.sh"
  ./pkg/prepkg.sh >> $LOG 2>&1
  echo "-- $(date +%T) finish OK"
else
  echo "-- $(date +%T) finish FAIL"
fi

echo "LOG: $LOG"
exit $grc
