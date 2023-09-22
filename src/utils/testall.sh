#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

grc=0
LOG=test.log
> $LOG

make distclean
make >> $LOG 2>&1

echo "== check" >> $LOG
./src/utils/mktestsetup.sh --force >> $LOG 2>&1
./bin/bdj4 --check_all >> $LOG 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "check: FAIL"
  grc=1
else
  echo "check: OK"
fi

if [[ $grc -eq 0 ]]; then
  echo "== dbtest" >> $LOG
  ./src/utils/mktestsetup.sh >> $LOG 2>&1
  # dbtest will rebuild the databases.
  ./src/utils/dbtest.sh --atimutagen >> $LOG 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "dbtest: atimutagen FAIL"
    grc=1
  else
    echo "dbtest: atimutagen OK"
  fi

  # dbtest will rebuild the databases.
  ./src/utils/dbtest.sh --atibdj4 >> $LOG 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "dbtest: atibdj4 FAIL"
    grc=1
  else
    echo "dbtest: atibdj4 OK"
  fi
else
  echo "dbtest not run"
fi

if [[ $grc -eq 0 ]]; then
  echo "== insttest" >> $LOG
  ./src/utils/mktestsetup.sh >> $LOG 2>&1
  # insttest will rebuild the databases.
  ./src/utils/insttest.sh >> $LOG 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "insttest: FAIL"
    grc=1
  else
    echo "insttest: OK"
  fi
fi

if [[ $grc -eq 0 ]]; then
  echo "== testsuite" >> $LOG
  ./src/utils/mktestsetup.sh >> $LOG 2>&1
  ./bin/bdj4 --testsuite >> $LOG 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "testsuite: FAIL"
    grc=1
  else
    echo "testsuite: OK"
  fi
else
  echo "testsuite not run"
fi

if [[ $grc -eq 0 ]]; then
  ./pkg/mkpkg.sh >> $LOG 2>&1
fi

echo "LOG: $LOG"
exit $grc
