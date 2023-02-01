#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

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
DATATOPDIR=${TARGETDIR}
if [[ $tag == macos ]]; then
  DATATOPDIR="$HOME/Library/Application Support/BDJ4"
fi
DATADIR="${DATATOPDIR}/data"
UNPACKDIR="${cwd}/tmp/bdj4-install"
UNPACKDIRBASE="${cwd}/tmp/bdj4-install${macdir}"
UNPACKDIRTMP="$UNPACKDIR.tmp"
LOG="tmp/insttest-log.txt"

currvers=$(pkglongvers)

hostname=$(hostname)
mconf=data/${hostname}/bdjconfig.txt

function mkBadPldance {
  tfn=$1

  awk 'BEGIN { flag = 0; }
      $0 == "DANCE" { flag = 0; }
      $0 == "..Viennese Waltz" { ++flag; }
      $0 == "..Weense wals" { ++flag; }
      $0 == "MAXPLAYTIME" { ++flag; }
      flag == 2 && ($0 == ".." || $0 == "..0") {
          $0 = "..15000"; flag = 0; }
      { print; }' \
      "$tfn" \
      > "$tfn.n"
  mv -f "$tfn.n" "$tfn"
}

function checkUpdaterClean {
  section=$1

  # audio adjust file should be installed if missing
  rm -f "$DATADIR/audioadjust.txt"
  # gtk-static.css file should be installed if missing
  rm -f "$DATADIR/gtk-static.css"
  # itunes-fields version number should be updated to version 2.
  fn="$DATADIR/itunes-fields.txt"
  sed -e 's/version 2/version 1/' "$fn" > "$fn.n"
  mv -f "$fn.n" "$fn"
  # standard rounds had bad data
  fn="$DATADIR/standardrounds.pldances"
  if [[ $section == nl ]]; then
    fn="$DATADIR/Standaardrondes.pldances"
  fi
  if [[ -f $fn ]]; then
    mkBadPldance "$fn"
  fi
  # queue dance had bad data
  fn="$DATADIR/QueueDance.pldances"
  if [[ $section == nl ]]; then
    fn="$DATADIR/wachtrijendans.pldances"
  fi
  if [[ -f $fn ]]; then
    mkBadPldance "$fn"
  fi
}

function checkInstallation {
  section=$1
  tname=$2
  tout=$(echo $3 | sed "s/\r//g")       # for windows
  trc=$4
  type=$5
  datafiles=$6

  chk=0
  res=0
  tcrc=1
  fin=F

  tcount=$(($tcount+1))

  res=$(($res+1))   # finish
  set BAD
  if [[ $tout != "" ]]; then
    set $tout
  fi
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

  if [[ $datafiles == y ]]; then
    res=$(($res+1))  # data dir
    if [[ $fin == T && -d "${DATADIR}" ]]; then
      chk=$(($chk+1))
    else
      echo "  no data directory"
    fi

    res=$(($res+1))  # data/profile00 dir
    if [[ $fin == T && -d "${DATADIR}/profile00" ]]; then
      chk=$(($chk+1))
    else
      echo "  no data/profile00 directory"
    fi

    res=$(($res+1))  # tmp dir
    if [[ $fin == T && -d "${DATATOPDIR}/tmp" ]]; then
      chk=$(($chk+1))
    else
      echo "  no tmp directory"
    fi

    res=$(($res+1))  # audioadjust.txt file
    if [[ $fin == T && -f "${DATADIR}/audioadjust.txt" ]]; then
      chk=$(($chk+1))
    else
      echo "  no audioadjust.txt file"
    fi

    res=$(($res+1))  # gtk-static.css file
    if [[ $fin == T && -f "${DATADIR}/gtk-static.css" ]]; then
      chk=$(($chk+1))
    else
      echo "  no gtk-static.css file"
    fi

    # automatic.pl file
    fna="${DATADIR}/automatic.pl"
    fnb="${DATADIR}/Automatisch.pl"
    if [[ $section == nl ]]; then
      temp="$fna"
      fna="$fnb"
      fnb="$temp"
    fi
    res=$(($res+1))
    if [[ -f ${fna} ]]; then
      if [[ -f ${fnb} ]]; then
        echo "  extra $(basename ${fnb})"
      else
        chk=$(($chk+1))
      fi
    else
      echo "  no $(basename ${fna})"
    fi

    res=$(($res+1))  # itunes-fields.txt file
    fn="${DATADIR}/itunes-fields.txt"
    if [[ $fin == T && -f $fn ]]; then
      grep 'version 2' "$fn" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  itunes-fields.txt file has wrong version"
      fi
    else
      echo "  no itunes-fields.txt file"
    fi

    res=$(($res+1))  # standardround.pldances file
    fn="${DATADIR}/QueueDance.pldances"
    if [[ $section == nl ]]; then
      fn="${DATADIR}/wachtrijendans.pldances"
    fi
    if [[ $fin == T && -f $fn ]]; then
      grep '^\.\.15000' "$fn" > /dev/null 2>&1
      rc=$?
      if [[ $rc -ne 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  $(basename "$fn") has bad vw maxplaytime"
      fi
    else
      echo "  no $(basename "$fn") file"
    fi

    res=$(($res+1))  # standardround.pldances file
    fn="${DATADIR}/standardrounds.pldances"
    if [[ $section == nl ]]; then
      fn="${DATADIR}/Standaardrondes.pldances"
    fi
    if [[ $fin == T && -f $fn ]]; then
      grep '^\.\.15000' "$fn" > /dev/null 2>&1
      rc=$?
      if [[ $rc -ne 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  $(basename "$fn") has bad vw maxplaytime"
      fi
    else
      echo "  no $(basename "$fn") file"
    fi
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

  if [[ $datafiles == y ]]; then
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
  fi

  if [[ -d "${DATADIR}" ]]; then
    c=$(ls -1 "${DATADIR}/asan*" 2>/dev/null | wc -l)
    if [[ $c -ne 0 ]]; then
      echo "ASAN files found"
      exit 1
    fi
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

function cleanInstTest {
  test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
  test -d "$TARGETTOPDIR" && rm -rf "$TARGETTOPDIR"
  test -d "$DATATOPDIR" && rm -rf "$DATATOPDIR"
}

function resetUnpack {
  test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
  cp -fpr "$UNPACKDIRTMP" "$UNPACKDIR"
}

test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
./pkg/mkpkg.sh --preskip --insttest
test -d "$UNPACKDIRTMP" && rm -rf "$UNPACKDIRTMP"
mv -f "$UNPACKDIR" "$UNPACKDIRTMP"

section=basic

cleanInstTest
resetUnpack

# main test db : rebuild of standard test database
tname=new-install-no-bdj3
out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer --cli --wait \
    --verbose --unattended --quiet \
    --msys \
    --targetdir "$TARGETTOPDIR" \
    --unpackdir "$UNPACKDIR" \
    )
rc=$?
checkInstallation $section $tname "$out" $rc n y
crc=$?

if [[ $crc -eq 0 ]]; then
  # standard re-install
  resetUnpack
  tname=re-install-no-bdj3
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer --cli --wait \
      --verbose --unattended --quiet \
      --msys \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      --reinstall \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc r y

  # standard update
  resetUnpack
  tname=update-no-bdj3
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer --cli --wait \
      --verbose --unattended --quiet \
      --msys \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc u y

  # update w/various update tasks
  # this should get installed as of version 4.1.0
  resetUnpack
  tname=update-chk-updater
  checkUpdaterClean $section
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer --cli --wait \
      --verbose --unattended --quiet \
      --msys \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc u y
fi

# install w/o data files
cleanInstTest
resetUnpack
tname=install-no-data
out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer --cli --wait \
    --verbose --unattended --quiet \
    --msys \
    --targetdir "$TARGETTOPDIR" \
    --unpackdir "$UNPACKDIR" \
    --nodatafiles \
    )
rc=$?
checkInstallation $section $tname "$out" $rc n n

section=nl
locale=nl_BE

cleanInstTest
resetUnpack

# main test db : rebuild of standard test database
tname=new-install-no-bdj3
out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer --cli --wait \
    --verbose --unattended --quiet \
    --msys \
    --targetdir "$TARGETTOPDIR" \
    --unpackdir "$UNPACKDIR" \
    --locale ${locale} \
    )
rc=$?
checkInstallation $section $tname "$out" $rc n y
crc=$?

if [[ $crc -eq 0 ]]; then
  resetUnpack
  tname=update-chk-updater

  # force the locale setting within BDJ4
  echo "nl_BE" > "$DATADIR/locale.txt"
  checkUpdaterClean $section
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer --cli --wait \
      --verbose --unattended --quiet \
      --msys \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      --locale ${locale} \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc u y
fi

cleanInstTest
test -d "$UNPACKDIRTMP" && rm -rf "$UNPACKDIRTMP"

echo "tests: $tcount pass: $pass fail: $fail"

exit $grc
