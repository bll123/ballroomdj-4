#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

. ./VERSION.txt

if [[ $DEVELOPMENT != dev ]]; then
  echo "Do not run $0 in a non-development environment"
  exit 1
fi

. src/utils/pkgnm.sh

tcount=0
pass=0
fail=0
grc=0
dump=F
keep=F
keepfirst=F
readonly=F
quiet=--quiet
tlocale=

while test $# -gt 0; do
  case $1 in
    --dump)
      dump=T
      ;;
    --keep)
      keep=T
      ;;
    --keepfirst)
      keepfirst=T
      ;;
    --readonly)
      readonly=T
      ;;
    --noquiet)
      quiet=""
      ;;
    --locale)
      shift
      tlocale=$1
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
TARGETALTDIR=${cwd}/tmp/BDJ4-alt
TARGETDIR=${TARGETTOPDIR}${macdir}
DATATOPDIR=${TARGETDIR}
if [[ $tag == macos ]]; then
  DATATOPDIR="$HOME/Library/Application Support/BDJ4"
fi
IMGDIR="${DATATOPDIR}/img"
DATADIR="${DATATOPDIR}/data"
HTTPDIR="${DATATOPDIR}/http"
UNPACKDIR="${cwd}/tmp/bdj4-install"
UNPACKDIRBASE="${cwd}/tmp/bdj4-install${macdir}"
UNPACKDIRSAVE="$UNPACKDIR.save"
MUSICDIR="${cwd}/test-music"
#ATI=libatimutagen
ATI=libatibdj4
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

  # 4.2.0 2023-3-5 autoselection values changed
  fn="$DATADIR/autoselection.txt"
  sed -e 's/version [2345]/version 1/;s/^\.\.[234]/..1/' "${fn}" > "${fn}.n"
  mv -f "${fn}.n" "${fn}"

  # audio adjust file should be installed if missing or wrong version
  fn="$DATADIR/audioadjust.txt"
  # rm -f "${fn}"
  sed -e 's/version [234]/version 1/;s/^\.\.[234]/..1/' "${fn}" > "${fn}.n"
  mv -f "${fn}.n" "${fn}"

  # itunes-fields version number should be updated to version 2.
  fn="$DATADIR/itunes-fields.txt"
  sed -e 's/version 2/version 1/' "${fn}" > "${fn}.n"
  mv -f "${fn}.n" "${fn}"

  # gtk-static version number should be updated to version 3.
  fn="$DATADIR/gtk-static.css"
  if [[ -f $fn ]]; then
    sed -e 's/version 3/version 1/' "${fn}" > "${fn}.n"
    mv -f "${fn}.n" "${fn}"
  fi

  # standard rounds had bad data
  fn="$DATADIR/standardrounds.pldances"
  if [[ $section == nl_BE || $section == nl_NL ]]; then
    fn="$DATADIR/Standaardrondes.pldances"
  fi
  if [[ $section == ru_RU ]]; then
    fn="$DATADIR/стандартные циклы.pldances"
  fi
  if [[ -f ${fn} ]]; then
    mkBadPldance "${fn}"
  fi
  # queue dance had bad data
  fn="$DATADIR/QueueDance.pldances"
  if [[ $section == nl_BE || $section == nl_NL ]]; then
    fn="$DATADIR/DansToevoegen.pl"
    # nl was renamed after the bad data situation
    rm -f "${fn}"
    fn="$DATADIR/DansToevoegen.pldances"
    rm -f "${fn}"
  fi
  if [[ $section == ru_RU ]]; then
    fn="$DATADIR/Добавить танец.pl"
  fi
  if [[ -f ${fn} ]]; then
    mkBadPldance "${fn}"
  fi

  # mobilemq.html version number should be updated to version 2.
  fn="$HTTPDIR/mobilemq.html"
  sed -e 's/VERSION 2/VERSION 1/' "${fn}" > "${fn}.n"
  mv -f "${fn}.n" "${fn}"
}

function checkInstallation {
  section=$1
  tname=$2
  tout=$(echo $3 | sed "s/\r//g")       # for windows
  trc=$4
  type=$5
  # datafiles may be 'y', 'n' or 'o' (only).
  datafiles=$6
  # target is an optional parameter, defaults to TARGETDIR
  target=${7:-${TARGETDIR}}

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
        if [[ $fin == T && ( $type == "u" || $type == "r" ) ]]; then
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
        if [[ $fin == T ]]; then
          res=$(($res+1))  # converted
          if [[ $1 == "0" ]]; then
            chk=$(($chk+1))
          else
            echo "  should not convert"
          fi
        fi
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
            echo "  incorrect version $1 $currvers"
          fi
        fi
        ;;
      *)
        # ignore any debugging
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

  if [[ $datafiles == y || $datafiles == o ]]; then
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

    fn=${DATADIR}/bdjconfig.txt
    res=$(($res+1))
    if [[ $fin == T && -f ${fn} ]]; then
      chk=$(($chk+1))
      res=$(($res+1))
      grep "^\.\.-1" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -ne 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  incorrect version in bdjconfig.txt"
      fi
    else
      echo "  no ${fn}"
    fi

    fn=${IMGDIR}/profile00/button_filter.svg
    res=$(($res+1))
    if [[ $fin == T && -f ${fn} ]]; then
      chk=$(($chk+1))
    else
      echo "  no ${fn}"
    fi

    fn=${DATADIR}/profile00/bdjconfig.txt
    res=$(($res+1))
    if [[ $fin == T && -f ${fn} ]]; then
      chk=$(($chk+1))
      res=$(($res+1))
      grep "^\.\.-1" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -ne 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  incorrect version in profile00/bdjconfig.txt"
      fi
    else
      echo "  no ${fn}"
    fi

    fn=${DATADIR}/${hostname}/bdjconfig.txt
    res=$(($res+1))
    if [[ $fin == T && -f ${fn} ]]; then
      chk=$(($chk+1))
      res=$(($res+1))

      # check for bad version
      grep "^\.\.-1" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -ne 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  incorrect version in ${hostname}/bdjconfig.txt"
      fi

      # check <hostname>/bdjconfig.txt for proper ati interface
      res=$(($res+1))
      grep "${ATI}" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        tval=$(grep "${ATI}" "${fn}")
        echo "  incorrect ati interface $tval"
      fi

      # check for null volume
      res=$(($res+1))
      grep "libvolnull" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -ne 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  null volume interface"
      fi

      # check for null pli
      res=$(($res+1))
      grep "libplinull" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -ne 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  null player interface"
      fi
    else
      echo "  no ${fn}"
    fi

    fn=${DATADIR}/${hostname}/profile00/bdjconfig.txt
    res=$(($res+1))
    if [[ $fin == T && -f ${fn} ]]; then
      chk=$(($chk+1))
      res=$(($res+1))
      grep "^\.\.-1" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -ne 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  incorrect version in ${hostname}/profile00/bdjconfig.txt"
      fi
    else
      echo "  no ${fn}"
    fi

    res=$(($res+1))  # tmp dir
    if [[ $fin == T && -d "${DATATOPDIR}/tmp" ]]; then
      chk=$(($chk+1))
    else
      echo "  no tmp directory"
    fi

    res=$(($res+1))  # img dir
    if [[ $fin == T && -d "${DATATOPDIR}/img" ]]; then
      chk=$(($chk+1))
    else
      echo "  no img directory"
    fi

    res=$(($res+1))  # img/profile00 dir
    if [[ $fin == T && -d "${DATATOPDIR}/img/profile00" ]]; then
      chk=$(($chk+1))
    else
      echo "  no img/profile00 directory"
    fi

    res=$(($res+1))
    if [[ $fin == T && -f "${DATATOPDIR}/img/profile00/button_add.svg" ]]; then
      chk=$(($chk+1))
    else
      echo "  no button_add.svg file"
    fi

    res=$(($res+1))  # http dir
    if [[ $fin == T && -d "${HTTPDIR}" ]]; then
      chk=$(($chk+1))
    else
      echo "  no http directory"
    fi

    # various http files
    res=$(($res+1))
    if [[ $fin == T && -f "${HTTPDIR}/led_on.svg" ]]; then
      chk=$(($chk+1))
    else
      echo "  no http/led_on.svg file"
    fi

    res=$(($res+1))
    if [[ $fin == T && -f "${HTTPDIR}/ballroomdj4.svg" ]]; then
      chk=$(($chk+1))
    else
      echo "  no http/ballroomdj4.svg file"
    fi

    res=$(($res+1))
    if [[ $fin == T && -f "${HTTPDIR}/favicon.ico" ]]; then
      chk=$(($chk+1))
    else
      echo "  no http/favicon.ico file"
    fi

    res=$(($res+1))
    if [[ $fin == T && -f "${HTTPDIR}/mobilemq.html" ]]; then
      chk=$(($chk+1))
    else
      echo "  no http/mobilemq.html file"
    fi

    res=$(($res+1))
    if [[ $fin == T && -f "${HTTPDIR}/mrc/dark/play.svg" ]]; then
      chk=$(($chk+1))
    else
      echo "  no http/mrc/dark/play.svg file"
    fi

    res=$(($res+1))  # audioadjust.txt file
    fn="${DATADIR}/audioadjust.txt"
    if [[ $fin == T && -f "${fn}" ]]; then
      grep 'version 4' "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        grep '^\.\.4' "${fn}" > /dev/null 2>&1
        rc=$?
        if [[ $rc -eq 0 ]]; then
          chk=$(($chk+1))
        else
          echo "  audioadjust.txt file has wrong version (a)"
        fi
      else
        echo "  audioadjust.txt file has wrong version (b)"
      fi
    else
      echo "  no audioadjust.txt file"
    fi

    res=$(($res+1))  # autoselection.txt file
    fn="${DATADIR}/autoselection.txt"
    if [[ $fin == T && -f "${fn}" ]]; then
      grep 'version 5' "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        grep '^\.\.5' "${fn}" > /dev/null 2>&1
        rc=$?
        if [[ $rc -eq 0 ]]; then
          chk=$(($chk+1))
        else
          echo "  autoselection.txt file has wrong version (a)"
        fi
      else
        echo "  autoselection.txt file has wrong version (b)"
      fi
    else
      echo "  no autoselection.txt file"
    fi

    # automatic.pl file
    fna="${DATADIR}/automatic.pl"
    fnb="${DATADIR}/Automatisch.pl"
    fnc="${DATADIR}/автоматически.pl"
    if [[ $section == nl_BE || $section == nl_NL ]]; then
      temp="${fna}"
      fna="${fnb}"
      fnb="$temp"
    fi
    if [[ $section == ru_RU ]]; then
      temp="${fna}"
      fna="${fnc}"
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
    if [[ $fin == T && -f ${fn} ]]; then
      grep 'version 2' "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  itunes-fields.txt file has wrong version"
      fi
    else
      echo "  no itunes-fields.txt file"
    fi

    res=$(($res+1))  # itunes-fields.txt file
    fn="${DATADIR}/gtk-static.css"
    if [[ $fin == T && -f ${fn} ]]; then
      grep 'version 3' "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  gtk-static.css file has wrong version"
      fi
    else
      echo "  no gtk-static.css file"
    fi

    res=$(($res+1))  # queuedance.pldances file
    fna="${DATADIR}/QueueDance.pldances"
    fnb="${DATADIR}/DansToevoegen.pldances"
    fnc="${DATADIR}/Добавить танец.pldances"
    if [[ $section == nl_BE || $section == nl_NL ]]; then
      temp="${fna}"
      fna="${fnb}"
      fnb="$temp"
    fi
    if [[ $section == ru_RU ]]; then
      temp="${fna}"
      fna="${fnc}"
      fnb="$temp"
    fi
    if [[ $fin == T && -f ${fna} ]]; then
      if [[ -f ${fnb} ]]; then
        echo "  extra $(basename ${fnb}) file"
      else
        grep '# version 3' "${fna}" > /dev/null 2>&1
        if [[ $rc -eq 0 ]]; then
          chk=$(($chk+1))
        else
          echo "  $(basename ${fna}) incorrect version"
        fi
      fi
    else
      echo "  no $(basename ${fna}) file"
    fi

    res=$(($res+1))  # queuedance.pl file
    fna="${DATADIR}/QueueDance.pl"
    fnb="${DATADIR}/DansToevoegen.pl"
    fnc="${DATADIR}/Добавить танец.pl"
    if [[ $section == nl_BE || $section == nl_NL ]]; then
      temp="${fna}"
      fna="${fnb}"
      fnb="$temp"
    fi
    if [[ $section == ru_RU ]]; then
      temp="${fna}"
      fna="${fnc}"
      fnb="$temp"
    fi
    if [[ $fin == T && -f ${fna} ]]; then
      if [[ -f ${fnb} ]]; then
        echo "  extra $(basename ${fnb}) file"
      else
        chk=$(($chk+1))
      fi
    else
      echo "  no $(basename ${fna}) file"
    fi

    res=$(($res+1))  # standardrounds.pldances file
    fn="${DATADIR}/standardrounds.pldances"
    if [[ $section == nl_BE || $section == nl_NL ]]; then
      fn="${DATADIR}/Standaardrondes.pldances"
    fi
    if [[ $section == ru_RU ]]; then
      fn="${DATADIR}/стандартные циклы.pldances"
    fi
    if [[ $fin == T && -f ${fn} ]]; then
      grep '# version 2' "${fn}" > /dev/null 2>&1
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  $(basename ${fn}) incorrect version"
      fi
    else
      echo "  no $(basename ${fn}) file"
    fi

    res=$(($res+1))  # volintfc.txt file removed
    fn="${DATADIR}/volintfc.txt"
    if [[ $fin == T && ! -f "${fn}" ]]; then
      chk=$(($chk+1))
    else
      echo "  $(basename ${fn}) exists"
    fi

    res=$(($res+1))  # playerintfc.txt file removed
    fn="${DATADIR}/playerintfc.txt"
    if [[ $fin == T && ! -f "${fn}" ]]; then
      chk=$(($chk+1))
    else
      echo "  $(basename ${fn}) exists"
    fi

    res=$(($res+1))  # audiotagintfc.txt file removed
    fn="${DATADIR}/audiotagintfc.txt"
    if [[ $fin == T && ! -f "${fn}" ]]; then
      chk=$(($chk+1))
    else
      echo "  $(basename ${fn}) exists"
    fi

    res=$(($res+1))  # mobilemq.html file
    fn="${HTTPDIR}/mobilemq.html"
    if [[ $fin == T && -f ${fn} ]]; then
      grep '<!-- VERSION 2' "${fn}" > /dev/null 2>&1
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  $(basename ${fn}) incorrect version"
      fi
    else
      echo "  no $(basename ${fn}) file"
    fi
  fi

  if [[ $datafiles != o ]]; then
    # main image files
    res=$(($res+1))
    if [[ $fin == T && -f "${target}/img/led_on.svg" ]]; then
      chk=$(($chk+1))
    else
      echo "  no img/led_on.svg file"
    fi

    res=$(($res+1))  # bin dir
    if [[ $fin == T && -d "${target}/bin" ]]; then
      chk=$(($chk+1))
    else
      echo "  no bin directory"
    fi

    res=$(($res+1))  # bdj4 exec
    if [[ $fin == T && -f "${target}/bin/bdj4${sfx}" ]]; then
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
  fi

  if [[ -d "${DATATOPDIR}" ]]; then
    c=$(ls -1 "${DATATOPDIR}/asan*" 2>/dev/null | wc -l)
    if [[ $c -ne 0 ]]; then
      echo "ASAN files found"
      exit 1
    fi
    c=$(ls -1 "${DATATOPDIR}/tmp/atimutagen*" 2>/dev/null | wc -l)
    if [[ $c -ne 0 ]]; then
      echo "atimutagen files found"
      exit 1
    fi
  fi

  c=$(ls -1 "${target}/core" 2>/dev/null | wc -l)
  if [[ $c -ne 0 ]]; then
    echo "core file found"
    exit 1
  fi
  if [[ $TARGETALTDIR != $target ]]; then
    c=$(ls -1 "${TARGETALTDIR}/core" 2>/dev/null | wc -l)
    if [[ $c -ne 0 ]]; then
      echo "core file found (b)"
      exit 1
    fi
  fi
  if [[ $TARGETTOPDIR != $target ]]; then
    c=$(ls -1 "${TARGETTOPDIR}/core" 2>/dev/null | wc -l)
    if [[ $c -ne 0 ]]; then
      echo "core file found (c)"
      exit 1
    fi
  fi

  if [[ $chk -eq $res ]]; then
    tcrc=0
    pass=$(($pass+1))
    echo "   $section $tname OK"
  else
    grc=1
    fail=$(($fail+1))
    echo "   $section $tname FAIL"
  fi

  if [[ $dump == T ]]; then
    echo "  rc: $trc"
    echo "  out:"
    echo $tout | sed 's/^/  /'
  fi

  return $tcrc
}

function cleanInstTest {
  test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
  test -d "$TARGETTOPDIR" && rm -rf "$TARGETTOPDIR"
  test -d "$TARGETALTDIR" && rm -rf "$TARGETALTDIR"
  test -d "$DATATOPDIR" && rm -rf "$DATATOPDIR"
}

function waitForInstallDirRemoval {
  count=0
  while test -d "$UNPACKDIR" && test $count -lt 30; do
    sleep 1
    count=$(($count+1))
  done
  if [[ -d "$UNPACKDIR" ]]; then
    echo "install-dir removal failed".
    exit 1
  fi
}

function resetInstallDir {
  rsync -aS "$UNPACKDIRSAVE/" "$UNPACKDIR"
}

# create the installation dir, and save it off
test -d "$UNPACKDIR" && rm -rf "$UNPACKDIR"
./pkg/mkpkg.sh --preskip --insttest --noclean
test -d "$UNPACKDIRSAVE" && rm -rf "$UNPACKDIRSAVE"
mv -f "$UNPACKDIR" "$UNPACKDIRSAVE"

section=basic

localecmd=
if [[ $tlocale != "" ]]; then
  localeopt=--locale
  locale=$tlocale
fi

echo "-- $(date +%T) creating test music"
./src/utils/mktestsetup.sh --force

c=$(ls -1 "${DATATOPDIR}/tmp/atimutagen*" 2>/dev/null | wc -l)
if [[ $c -ne 0 ]]; then
  echo "atimutagen files found after mktestsetup.sh"
  exit 1
fi

cleanInstTest

if [[ $readonly == F ]]; then
  # main test db : rebuild of standard test database
  resetInstallDir
  tname=new-install
  echo "== $section $tname"
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
      --nomutagen \
      --ati ${ATI} \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      --musicdir "$MUSICDIR" \
      $localeopt $locale \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc n y
  crc=$?

  if [[ $keepfirst == T ]]; then
    exit 1
  fi
  waitForInstallDirRemoval
fi

if [[ $readonly == F && $crc -eq 0 ]]; then
  # standard re-install
  resetInstallDir
  tname=re-install
  echo "== $section $tname"
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
      --nomutagen \
      --ati ${ATI} \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      --musicdir "$MUSICDIR" \
      --reinstall \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc r y
  waitForInstallDirRemoval

  # standard update
  resetInstallDir
  tname=update
  echo "== $section $tname"
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
      --nomutagen \
      --ati ${ATI} \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      --musicdir "$MUSICDIR" \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc u y
  waitForInstallDirRemoval

  # update w/various update tasks
  # this should get installed as of version 4.1.0
  waitForInstallDirRemoval
  resetInstallDir
  tname=update-chk-updater
  echo "== $section $tname"
  checkUpdaterClean $section
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
      --nomutagen \
      --ati ${ATI} \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      --musicdir "$MUSICDIR" \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc u y
  waitForInstallDirRemoval
fi

if [[ $readonly == F ]]; then
  if [[ $tag == linux || $platform == windows ]]; then
    # alternate installation (linux, windows)
    tname=alt-install
    echo "== $section $tname"
    out=$(cd "$TARGETTOPDIR";./bin/bdj4 --bdj4altinst \
        --verbose --unattended ${quiet} \
        --ati ${ATI} \
        --targetdir "$TARGETALTDIR" \
        --musicdir "$MUSICDIR" \
        )
    rc=$?
    checkInstallation $section $tname "$out" $rc n o "${TARGETALTDIR}"

    # alternate installation (linux, windows)
    tname=alt-install-reinstall
    echo "== $section $tname"
    out=$(cd "$TARGETTOPDIR";./bin/bdj4 --bdj4altinst \
        --verbose --unattended ${quiet} \
        --ati ${ATI} \
        --targetdir "$TARGETALTDIR" \
        --musicdir "$MUSICDIR" \
        --reinstall \
        )
    rc=$?
    checkInstallation $section $tname "$out" $rc r o "${TARGETALTDIR}"

    # alternate installation (linux, windows)
    tname=alt-install-update
    echo "== $section $tname"
    out=$(cd "$TARGETTOPDIR";./bin/bdj4 --bdj4altinst \
        --verbose --unattended ${quiet} \
        --ati ${ATI} \
        --targetdir "$TARGETALTDIR" \
        --musicdir "$MUSICDIR" \
        )
    rc=$?
    checkInstallation $section $tname "$out" $rc u o "${TARGETALTDIR}"
  fi
fi

if [[ T == T ]]; then
  # install w/o data files
  cleanInstTest
  resetInstallDir
  tname=install-readonly
  echo "== $section $tname"
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
      --nomutagen \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      --readonly \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc n n
  waitForInstallDirRemoval
fi

if [[ $readonly == T ]]; then
  exit 1
fi

section=nl_BE
locale=nl_BE

cleanInstTest
resetInstallDir

# main test db : rebuild of standard test database, nl_BE
tname=new-install
echo "== $section $tname"
out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
    --verbose --unattended ${quiet} \
    --nomutagen \
    --ati ${ATI} \
    --targetdir "$TARGETTOPDIR" \
    --unpackdir "$UNPACKDIR" \
    --musicdir "$MUSICDIR" \
    --locale ${locale} \
    )
rc=$?
checkInstallation $section $tname "$out" $rc n y
crc=$?
waitForInstallDirRemoval

if [[ $crc -eq 0 ]]; then
  resetInstallDir
  tname=update-chk-updater
  echo "== $section $tname"

  checkUpdaterClean $section
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
      --nomutagen \
      --ati ${ATI} \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      --musicdir "$MUSICDIR" \
      --locale ${locale} \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc u y
  waitForInstallDirRemoval
fi

if [[ T == T ]]; then
  if [[ $tag == linux || $platform == windows ]]; then
    # alternate installation (linux, windows)
    tname=alt-install
    echo "== $section $tname"
    out=$(cd "$TARGETTOPDIR";./bin/bdj4 --bdj4altinst \
        --verbose --unattended ${quiet} \
        --ati ${ATI} \
        --targetdir "$TARGETALTDIR" \
        --musicdir "$MUSICDIR" \
        --locale ${locale} \
        )
    rc=$?
    checkInstallation $section $tname "$out" $rc n o "${TARGETALTDIR}"
  fi
fi

section=nl_NL
locale=nl_NL

cleanInstTest
resetInstallDir

# main test db : rebuild of standard test database, nl_NL
tname=new-install
echo "== $section $tname"
out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
    --verbose --unattended ${quiet} \
    --nomutagen \
    --ati ${ATI} \
    --targetdir "$TARGETTOPDIR" \
    --unpackdir "$UNPACKDIR" \
    --musicdir "$MUSICDIR" \
    --locale ${locale} \
    )
rc=$?
checkInstallation $section $tname "$out" $rc n y
crc=$?
waitForInstallDirRemoval

section=ru_RU
locale=ru_RU

cleanInstTest
resetInstallDir

# main test db : rebuild of standard test database, nl_BE
tname=new-install
echo "== $section $tname"
out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
    --verbose --unattended ${quiet} \
    --nomutagen \
    --ati ${ATI} \
    --targetdir "$TARGETTOPDIR" \
    --unpackdir "$UNPACKDIR" \
    --musicdir "$MUSICDIR" \
    --locale ${locale} \
    )
rc=$?
checkInstallation $section $tname "$out" $rc n y
crc=$?
waitForInstallDirRemoval

if [[ $crc -eq 0 ]]; then
  resetInstallDir
  tname=update-chk-updater
  echo "== $section $tname"

  checkUpdaterClean $section
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
      --nomutagen \
      --ati ${ATI} \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      --musicdir "$MUSICDIR" \
      --locale ${locale} \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc u y
  waitForInstallDirRemoval
fi

if [[ T == T ]]; then
  if [[ $tag == linux || $platform == windows ]]; then
    # alternate installation (linux, windows)
    tname=alt-install
    echo "== $section $tname"
    out=$(cd "$TARGETTOPDIR";./bin/bdj4 --bdj4altinst \
        --verbose --unattended ${quiet} \
        --ati ${ATI} \
        --targetdir "$TARGETALTDIR" \
        --musicdir "$MUSICDIR" \
        --locale ${locale} \
        )
    rc=$?
    checkInstallation $section $tname "$out" $rc n o "${TARGETALTDIR}"
  fi
fi

if [[ $keep == F ]]; then
  cleanInstTest
  test -d "$UNPACKDIRSAVE" && rm -rf "$UNPACKDIRSAVE"
fi

echo "tests: $tcount pass: $pass fail: $fail"

exit $grc
