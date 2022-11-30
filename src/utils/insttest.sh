#!/bin/bash

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

. src/utils/pkgnm.sh

tcount=0
pass=0
fail=0
grc=0
verbose=F

while test $# -gt 0; do
  case $1 in
    --verbose)
      verbose=T
      ;;
  esac
  shift
done

cwd=$(pwd)

systype=$(uname -s)
case $systype in
  Linux)
    tag=linux
    platform=unix
    libvol=libvolpa
    libpli=libplivlc
    macdir=""
    sfx=
    ;;
  Darwin)
    tag=macos
    platform=unix
    libvol=libvolmac
    libpli=libplivlc
    macdir=/Contents/MacOS
    sfx=
    ;;
  MINGW64*)
    tag=win64
    platform=windows
    libvol=libvolwin
    libpli=libplivlc
    macdir=""
    sfx=.exe
    ;;
esac

TARGETTOPDIR=${cwd}/tmp/BDJ4
TARGETDIR=${TARGETTOPDIR}${macdir}
DATADIR=${TARGETDIR}
if [[ $tag == macos ]]; then
  DATADIR="$HOME/Library/Application Support/BDJ4"
fi
UNPACKDIR="${cwd}/tmp/bdj4-install"
UNPACKDIRTMP="$UNPACKDIR.tmp"
LOG="tmp/insttest-log.txt"

currvers=$(pkglongvers)

hostname=$(hostname)
mconf=data/${hostname}/bdjconfig.txt

function check {
  section=$1
  tname=$2
  tout=$(echo $3 | sed "s/\r//g")       # for windows
  trc=$4
  type=$5

  chk=0
  res=0
  tcrc=1
  fin=F

  tcount=$(($tcount+1))

  res=$(($res+1))   # finish
  set $tout
  while test $# -gt 0; do
    case $1 in
      finish)
        shift
        if [[ $1 == "OK" ]]; then
          chk=$(($chk+1))
          fin=T
        else
          echo "  install not finished"
        fi
        ;;
      update)
        shift
        if [[ $fin == T && $type == "u" ]]; then
          res=$(($res+1))  # update
          if [[ $1 == "1" ]]; then
            chk=$(($chk+1))
          else
            echo "  should be an update"
          fi
        fi
        ;;
      new-install)
        shift
        if [[ $fin == T && $type == "n" ]]; then
          res=$(($res+1))  # new-install
          if [[ $1 == "1" ]]; then
            chk=$(($chk+1))
          else
            echo "  should be a new install"
          fi
        fi
        ;;
      re-install)
        shift
        if [[ $fin == T && $type == "r" ]]; then
          res=$(($res+1))  # re-install
          if [[ $1 == "1" ]]; then
            chk=$(($chk+1))
          else
            echo "  should be a re-install"
          fi
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
        if [[ $1 != "x" ]]; then
          res=$(($res+1))  # old-version
          if [[ $fin == T && $1 == $currvers ]]; then
            chk=$(($chk+1))
          else
            echo "  incorrect version"
          fi
        fi
        ;;
    esac
    shift
  done

  res=$(($res+1))  # return code
  if [[ $trc -eq 0 ]]; then
    chk=$(($chk+1))
  else
    echo "  installer: bad return code"
  fi
  res=$(($res+1))  # data dir
  if [[ $fin == T && -d "${DATADIR}/data" ]]; then
    chk=$(($chk+1))
  else
    echo "  no data directory"
  fi
  res=$(($res+1))  # data/profile00 dir
  if [[ $fin == T && -d "${DATADIR}/data/profile00" ]]; then
    chk=$(($chk+1))
  else
    echo "  no data/profile00 directory"
  fi
  res=$(($res+1))  # tmp dir
  if [[ $fin == T && -d "${DATADIR}/tmp" ]]; then
    chk=$(($chk+1))
  else
    echo "  no tmp directory"
  fi
  res=$(($res+1))  # bin dir
  if [[ $fin == T && -d "${TARGETDIR}/bin" ]]; then
    chk=$(($chk+1))
  else
    echo "  no bin directory"
  fi
  res=$(($res+1))  # bdj4 exec
  if [[ $fin == T && -f "${TARGETDIR}/bin/bdj4${sfx}" ]]; then
    chk=$(($chk+1))
  else
    echo "  no bdj4 executable"
  fi

  lvol=$(sed -n -e '/^VOLUME/ { n; s/^\.\.//; p ; }' $mconf)
  lpli=$(sed -n -e '/^PLAYER/ { n; s/^\.\.//; p ; }' $mconf)

  res=$(($res+1))  # volume lib
  if [[ $fin == T && $libvol == $lvol ]]; then
    chk=$(($chk+1))
  else
    echo "  volume library not set correctly"
  fi
  res=$(($res+1))  # pli lib
  if [[ $fin == T && $libpli == $lpli ]]; then
    chk=$(($chk+1))
  else
    echo "  pli library not set correctly"
  fi

  if [[ $chk -eq $res ]]; then
    tcrc=0
    pass=$(($pass+1))
    echo "$section $tname OK"
  else
    grc=1
    fail=$(($fail+1))
    echo "$section $tname FAIL"
    if [[ $verbose == T ]]; then
      echo "  rc: $trc"
      echo "  out:"
      echo $tout | sed 's/^/  /'
    fi
  fi
  return $tcrc
}

function resetUnpack {
  test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
  cp -fpr "$UNPACKDIR.tmp" "$UNPACKDIR"
}

test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
./pkg/mkpkg.sh --preskip --insttest
test -d "$UNPACKDIRTMP" && rm -rf "$UNPACKDIRTMP"
cp -fpr "$UNPACKDIR" "$UNPACKDIRTMP"

section=basic

# main test db : rebuild of standard test database
tname=new-install-no-bdj3
test -d $TARGETTOPDIR && rm -rf $TARGETTOPDIR
out=$(./bin/bdj4 --bdj4installer --cli --wait \
    --verbose --unattended --quiet \
    --msys \
    --targetdir "$TARGETTOPDIR" \
    --unpackdir "$UNPACKDIR" \
    )
rc=$?
check $section $tname "$out" $rc n
crc=$?

if [[ $crc -eq 0 ]]; then
  resetUnpack
  tname=re-install-no-bdj3
  out=$(./bin/bdj4 --bdj4installer --cli --wait \
      --verbose --unattended --quiet \
      --msys \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      --reinstall \
      )
  rc=$?
  check $section $tname "$out" $rc r

  resetUnpack
  tname=update-no-bdj3
  out=$(./bin/bdj4 --bdj4installer --cli --wait \
      --verbose --unattended --quiet \
      --msys \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      )
  rc=$?
  check $section $tname "$out" $rc u
fi

test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
test -d "$UNPACKDIRTMP" && rm -rf "$UNPACKDIRTMP"

echo "tests: $tcount pass: $pass fail: $fail"

exit $grc
