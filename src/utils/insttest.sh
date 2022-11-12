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

cwd=$(pwd)

TARGETDIR=${cwd}/tmp/BDJ4
UNPACKDIR="${cwd}/tmp/bdj4-install"
UNPACKDIRTMP="$UNPACKDIR.tmp"
LOG="tmp/insttest-log.txt"

hostname=$(hostname)
mconf=data/${hostname}/bdjconfig.txt

systype=$(uname -s)
case $systype in
  Linux)
    tag=linux
    platform=unix
    libvol=libvolpa
    libpli=libplivlc
    sfx=
    ;;
  Darwin)
    tag=macos
    platform=unix
    libvol=libvolmac
    libpli=libplivlc
    sfx=
    ;;
  MINGW64*)
    tag=win64
    platform=windows
    libvol=libvolwin
    libpli=libplivlc
    sfx=.exe
    ;;
esac

function check {
  tname=$1
  tout=$2
  trc=$3
  type=$4
  chk=0
  grc=1

  tcount=$(($tcount+1))

  set $tout
  while test $# -gt 0; do
    case $1 in
      finish)
        shift
        if [[ $1 == "OK" ]]; then
          chk=$(($chk+1))
        fi
        ;;
      update)
        shift
        if [[ $type == "u" && $1 == "1" ]]; then
          chk=$(($chk+1))
        fi
        ;;
      new-install)
        shift
        if [[ $type == "n" && $1 == "1" ]]; then
          chk=$(($chk+1))
        fi
        ;;
      re-install)
        shift
        if [[ $type == "r" && $1 == "1" ]]; then
          chk=$(($chk+1))
        fi
        ;;
      converted)
        shift
        ;;
      bdj3-version)
        shift
        ;;
      old-version)
        shift
        ;;
    esac
    shift
  done

  if [[ $rc -eq 0 ]]; then
    chk=$(($chk+1))
  fi
  if [[ -d "${TARGETDIR}/data" ]]; then
    chk=$(($chk+1))
  fi
  if [[ -d "${TARGETDIR}/tmp" ]]; then
    chk=$(($chk+1))
  fi
  if [[ -d "${TARGETDIR}/bin" ]]; then
    chk=$(($chk+1))
  fi
  if [[ -f "${TARGETDIR}/bin/bdj4${sfx}" ]]; then
    chk=$(($chk+1))
  fi

  lvol=$(sed -n -e '/^VOLUME/ { n; s/^\.\.//; p ; }' $mconf)
  lpli=$(sed -n -e '/^PLAYER/ { n; s/^\.\.//; p ; }' $mconf)

  if [[ $libvol == $lvol ]]; then
    chk=$(($chk+1))
  fi
  if [[ $libpli == $lpli ]]; then
    chk=$(($chk+1))
  fi

  if [[ $chk -eq 9 ]]; then
    grc=0
    pass=$(($pass+1))
    echo "$tname OK"
  else
    fail=$(($fail+1))
    echo "$tname FAIL"
  fi
  return $grc
}

function resetUnpack {
  test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
  cp -fpr "$UNPACKDIR.tmp" "$UNPACKDIR"
}

test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
./pkg/mkpkg.sh --preskip --insttest
test -d "$UNPACKDIRTMP" && rm -rf "$UNPACKDIRTMP"
cp -fpr "$UNPACKDIR" "$UNPACKDIRTMP"

# main test db : rebuild of standard test database
tname=new-install-no-bdj3
test -d $TARGETDIR && rm -rf $TARGETDIR
out=$(./bin/bdj4 --bdj4install --cli --verbose --wait --unattended --quiet \
    --targetdir "$TARGETDIR" \
    --unpackdir "$UNPACKDIR" \
    )
rc=$?
check $tname "$out" $rc n

resetUnpack
tname=re-install-no-bdj3
out=$(./bin/bdj4 --bdj4install --cli --verbose --wait --unattended --quiet \
    --targetdir "$TARGETDIR" \
    --unpackdir "$UNPACKDIR" \
    --reinstall \
    )
rc=$?
check $tname "$out" $rc r

resetUnpack
tname=update-no-bdj3
out=$(./bin/bdj4 --bdj4install --cli --verbose --wait --unattended --quiet \
    --targetdir "$TARGETDIR" \
    --unpackdir "$UNPACKDIR" \
    )
rc=$?
check $tname "$out" $rc u

test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
test -d "$UNPACKDIRTMP" && rm -rf "$UNPACKDIRTMP"

echo "tests: $tcount pass: $pass fail: $fail"

exit $rc
