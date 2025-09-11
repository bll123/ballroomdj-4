#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
  shift
done

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
  MINGW64*)
    os=win64
    platform=windows
    ;;
  MINGW32*)
    echo "Platform not supported"
    exit 1
    ;;
esac

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


function runTestSuite {
  pli=$1
  vol=$2

  echo "-- $(date +%T) make test setup"
  pliargs="--pli $pli"
  if [[ $os == linux && $pli == VLC3 ]]; then
    # this may change later
    pliargs=""
  fi
  volargs=""
  if [[ $vol != "" ]]; then
    volargs="--vol $vol"
  fi
  ./src/utils/mktestsetup.sh --force $pliargs $volargs >> $LOG 2>&1
  echo "-- $(date +%T) testsuite $pli $vol" >> $LOG
  echo "-- $(date +%T) testsuite $pli $vol"
  if [[ $os == macos ]]; then
    # macos has been having issues with the volume getting set to 0.
    /usr/bin/osascript -e "set volume without output muted"
    /usr/bin/osascript -e "set volume output volume 70"
  fi
  # the pliargs need to be passed to the bdj4 launcher
  # as macos and windows will modify their paths for vlc3/4.
  ./bin/bdj4 --testsuite $pliargs >> $LOG 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "-- $(date +%T) testsuite $pli $vol FAIL"
  else
    echo "-- $(date +%T) testsuite $pli $vol OK"
  fi
  return $rc
}

if [[ $TBUILD == T ]]; then
  (
    cd src
    case ${pn_dist} in
      -opensuse*)
        # change this in utils/pkg.sh also
        time make CC=gcc-13 CXX=g++-13
        ;;
      *)
        time make
        ;;
    esac
  ) >> $LOG 2>&1

  echo "-- $(date +%T) warnings"
  # 'warning generated' the compiler's display of the warning count.
  # windows has a multitude of warnings in check.h about %jd
  # mongoose.c is a third-party library.
  grep warning $LOG |
      grep -v 'check\.h' |
      grep -v 'mongoose\.c' |
      grep -v 'warning generated'

  # for windows, make sure the libraries in plocal are up to date
  # on linux, make sure the localization is up to date.
  ./pkg/prepkg.sh
fi

if [[ $TCHECK == T ]]; then
  case $pn_tag in
    linux|macos)
      cdir=${XDG_CACHE_HOME:-$HOME/.cache}
      cachedir="${cdir}/BDJ4"
      cdir=${XDG_CONFIG_HOME:-$HOME/.config}
      confdir="${cdir}/BDJ4"
      ;;
    win64)
      # run under msys2
      cachedir="${ORIGINAL_TEMP}/BDJ4"
      confdir="${ORIGINAL_TEMP}/../../Roaming/BDJ4"
      ;;
  esac
  tfn="$cachedir/volreg.txt"
  if [[ -d ${cachedir} && -f $tfn ]]; then
    rm -f $tfn
  fi
  if [[ -d ${confdir} ]]; then
    rm -f ${confdir}/*dev*
  fi
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
    echo "-- dbtest " >> $LOG
    echo "-- $(date +%T) dbtest"
    ./src/utils/dbtest.sh >> $LOG 2>&1
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "-- $(date +%T) dbtest FAIL"
      grc=1
    else
      echo "-- $(date +%T) dbtest OK"
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
  TESTVLC3ON=T
  if [[ $grc -eq 0 && $TESTVLC3ON == T ]]; then
    runTestSuite VLC3
    rc=$?
    if [[ $rc -ne 0 ]]; then
      grc=1
    fi
  fi

  # the pipewire volume interface is only tested on linux
  # need to modify this to check if the pipewire interface is available
  # on the particular linux platform.
  TESTPWON=F
  if [[ $grc -eq 0 && $TESTPWON == T && $os == linux ]]; then
    runTestSuite VLC3 pipewire
    rc=$?
    if [[ $rc -ne 0 ]]; then
      grc=1
    fi
  fi

  # for the moment, only test gstreamer on linux
  TESTGSTON=T
  if [[ $grc -eq 0 && $TESTGSTON == T && $os == linux ]]; then
    runTestSuite GST
    rc=$?
    if [[ $rc -ne 0 ]]; then
      grc=1
    fi
  fi

  # windows media player
  TESTWINMPON=T
  if [[ $grc -eq 0 && $TESTWINMPON == T && $os == window ]]; then
    runTestSuite WINMP
    rc=$?
    if [[ $rc -ne 0 ]]; then
      grc=1
    fi
  fi

  # for the time being, do not run the VLC 4 tests. (2024-6)
  # VLC 4 has no release date scheduled.
  TESTVLC4ON=F
  if [[ $grc -eq 0 && $TESTVLC4ON == T && $os != linux ]]; then
    runTestSuite VLC4
    rc=$?
    if [[ $rc -ne 0 ]]; then
      grc=1
    fi
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
