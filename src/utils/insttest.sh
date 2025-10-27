#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
    libpli=libpliwinmp      # default
    macdir=""
    sfx=.exe
    ;;
esac

DEVTMP=devel/tmp
TARGETTOPDIR=${cwd}/${DEVTMP}/BDJ4dev
TARGETTOPALTDIR=${cwd}/${DEVTMP}/BDJ4altdev
if [[ $tag == macos ]]; then
  TARGETTOPDIR=${cwd}/${DEVTMP}/BDJ4dev.app
  TARGETTOPALTDIR=${cwd}/${DEVTMP}/BDJ4altdev.app
fi
TARGETDIR=${TARGETTOPDIR}${macdir}
TARGETALTDIR=${TARGETTOPALTDIR}${macdir}

DATATOPDIR=${TARGETDIR}
DATATOPALTDIR=${TARGETDIR}
if [[ $tag == macos ]]; then
  DATATOPDIR="$HOME/Library/Application Support/BDJ4dev"
  DATATOPALTDIR="$HOME/Library/Application Support/BDJ4altdev"
fi

IMGDIR="${DATATOPDIR}/img"
DATADIR="${DATATOPDIR}/data"
HTTPDIR="${DATATOPDIR}/http"
UNPACKDIR="${cwd}/${DEVTMP}/bdj4-install"
UNPACKDIRBASE="${cwd}/${DEVTMP}/bdj4-install${macdir}"
UNPACKDIRSAVE="$UNPACKDIR.save"
LOG="${DEVTMP}/insttest-log.txt"

fn="templates/ds-audioid-list.txt"
AUDIOIDLISTVER=$(grep "^# version [1-9]" "${fn}" | sed 's,[^0-9],,g')
fn="templates/bdjuri.txt"
BDJURIVER=$(grep "^# version [1-9]" "${fn}" | sed 's,[^0-9],,g')
fn="templates/itunes-fields.txt"
ITUNESFIELDSVER=$(grep "^# version [1-9]" "${fn}" | sed 's,[^0-9],,g')
fn="templates/sortopt.txt"
SORTOPTVER=$(grep "^# version [1-9]" "${fn}" | sed 's,[^0-9],,g')
fn="templates/gtk-static.css"
GTKSTATICVER=$(grep "version [1-9]" "${fn}" | sed 's,[^0-9],,g')
fn="templates/autoselection.txt"
AUTOSELVER=$(grep "^# version [1-9]" "${fn}" | sed 's,[^0-9],,g')
fn="templates/audioadjust.txt"
AUDIOADJVER=$(grep "^# version [1-9]" "${fn}" | sed 's,[^0-9],,g')
fn="templates/mobilemq.html"
MOBILEMQVER=$(grep "<!-- VERSION [1-9]" "${fn}" | sed 's,[^0-9],,g')
fn="templates/standardrounds.pldances"
STDROUNDVER=$(grep "^# version [1-9]" "${fn}" | sed 's,[^0-9],,g')
fn="templates/QueueDance.pldances"
QDANCEVER=$(grep "^# version [1-9]" "${fn}" | sed 's,[^0-9],,g')

currvers=$(pkglongvers)

hostname=$(hostname)
mconf=${DATADIR}/${hostname}/bdjconfig.txt

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
  sed -e "s/version [2-9]/version $(($AUTOSELVER-1))/" "${fn}" > "${fn}.n"
  mv -f "${fn}.n" "${fn}"

  # audio adjust file should be installed if missing or wrong version
  fn="$DATADIR/audioadjust.txt"
  sed -e "s/version [2-9]/version $(($AUDIOADJVER-1))/" "${fn}" > "${fn}.n"
  mv -f "${fn}.n" "${fn}"

  # ds-audioid.txt file should be installed if missing
  fn="$DATADIR/profile00/ds-audioid.txt"
  rm -f "${fn}"

  # img/profile00/indicator_timer.svg file should be installed
  fn="${DATATOPDIR}/img/profile00/indicator_timer.svg"
  rm -f "${fn}"

  # ds-audioid-list.txt version number should be updated
  fn="$DATADIR/profile00/ds-audioid-list.txt"
  sed -e "s/version [2-9]/version $(($AUDIOIDLISTVER-1))/" "${fn}" > "${fn}.n"
  mv -f "${fn}.n" "${fn}"

  # bdjconfig.q4.txt file should be installed if missing
  fn="$DATADIR/profile00/bdjconfig.q4.txt"
  rm -f "${fn}"

  # itunes-fields version number should be updated
  fn="$DATADIR/itunes-fields.txt"
  sed -e "s/version [2-9]/version $(($ITUNESFIELDSVER-1))/" "${fn}" > "${fn}.n"
  mv -f "${fn}.n" "${fn}"

  # sortopt version number should be updated
  fn="$DATADIR/sortopt.txt"
  sed -e "s/version [2-9]/version $(($SORTOPTVER-1))/" "${fn}" > "${fn}.n"
  mv -f "${fn}.n" "${fn}"

  # gtk-static version number should be updated to version 4.
  fn="$DATADIR/gtk-static.css"
  if [[ -f $fn ]]; then
    sed -e "s/version [2-9]/version $(($GTKSTATICVER-1))/" "${fn}" > "${fn}.n"
    mv -f "${fn}.n" "${fn}"
  fi

  # standard rounds had bad data
  fn="$DATADIR/standardrounds.pldances"
  if [[ $section == de_DE ]]; then
    fn="${DATADIR}/Standardrunden.pldances"
  fi
  if [[ $section == es_ES ]]; then
    fn="${DATADIR}/estándares.pldances"
  fi
  if [[ $section == fi_FI ]]; then
    fn="${DATADIR}/vakiokierrokset.pldances"
  fi
  if [[ $section == fr_FR ]]; then
    fn="${DATADIR}/les rounds standards.pldances"
  fi
  if [[ $section == it_IT ]]; then
    fn="${DATADIR}/giri standard.pldances"
  fi
  if [[ $section == ja_JP ]]; then
    fn="${DATADIR}/スタンダードラウンド.pldances"
  fi
  if [[ $section == nl_BE || $section == nl_NL ]]; then
    fn="$DATADIR/Standaardrondes.pldances"
  fi
  if [[ $section == pl_PL ]]; then
    fn="${DATADIR}/rundy standardowe.pldances"
  fi
  if [[ $section == ru_RU ]]; then
    fn="$DATADIR/стандартные циклы.pldances"
  fi
  if [[ -f ${fn} ]]; then
    mkBadPldance "${fn}"
  fi

  # queue dance had bad data
  fn="$DATADIR/QueueDance.pldances"
  if [[ $section == de_DE ]]; then
    fn="${DATADIR}/Schlangentanz.pldances"
  fi
  if [[ $section == es_ES ]]; then
    fn="${DATADIR}/Danza en cola.pldances"
  fi
  if [[ $section == fi_FI ]]; then
    fn="$DATADIR/Jonotanssi.pldances"
  fi
  if [[ $section == fr_FR ]]; then
    fn="$DATADIR/Danse en file d'attente.pldances"
  fi
  if [[ $section == it_IT ]]; then
    fn="${DATADIR}/Ballo in coda.pldances"
  fi
  if [[ $section == ja_JP ]]; then
    fn="${DATADIR}/キューダンス.pldances"
  fi
  if [[ $section == nl_BE || $section == nl_NL ]]; then
    fn="$DATADIR/Dans In Wachtrij.pldances"
  fi
  if [[ $section == pl_PL ]]; then
    fn="${DATADIR}/Taniec w kolejce.pldances"
  fi
  if [[ $section == ru_RU ]]; then
    fn="$DATADIR/Танец в очередь.pldances"
  fi
  if [[ -f ${fn} ]]; then
    mkBadPldance "${fn}"
  fi

  # mobilemq.html version number should be updated
  fn="$HTTPDIR/mobilemq.html"
  sed -e "s/VERSION [2-9]/VERSION $(($MOBILEMQVER-1))/" "${fn}" > "${fn}.n"
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
  # datatop is an optional parameter, defaults to DATATOPDIR
  datatop=${8:-${DATATOPDIR}}

  chk=0
  res=0
  tcrc=1
  fin=F

  tcount=$(($tcount+1))

  set BAD
  if [[ $tout != "" ]]; then
    set $tout
  fi
  while test $# -gt 0; do
    case $1 in
      finish)
        shift
        res=$(($res+1))   # finish
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

    res=$(($res+1))  # data/profile01 dir
    if [[ $fin == T && ! -d "${DATADIR}/profile01" ]]; then
      chk=$(($chk+1))
    else
      echo "  have data/profile01 directory"
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

    fn=${DATADIR}/profile00/bdjconfig.q4.txt
    res=$(($res+1))
    if [[ $fin == T && -f ${fn} ]]; then
      chk=$(($chk+1))
    else
      echo "  no ${fn}"
    fi

    fn=${DATADIR}/profile00/ds-audioid.txt
    res=$(($res+1))
    if [[ $fin == T && -f ${fn} ]]; then
      chk=$(($chk+1))
    else
      echo "  no ${fn}"
    fi

    fn=${DATATOPDIR}/img/profile00/indicator_timer.svg
    res=$(($res+1))
    if [[ $fin == T && -f ${fn} ]]; then
      chk=$(($chk+1))
    else
      echo "  no ${fn}"
    fi

    fn=${DATADIR}/ds-audioid.txt
    res=$(($res+1))
    if [[ $fin == T && ! -f ${fn} ]]; then
      chk=$(($chk+1))
    else
      echo "  ${fn} should not be present"
    fi

    res=$(($res+1))  # ds-audioid-list file
    fn="${DATADIR}/profile00/ds-audioid-list.txt"
    if [[ $fin == T && -f ${fn} ]]; then
      tver=$(grep "version ${AUDIOIDLISTVER}" "${fn}")
      grep "version ${AUDIOIDLISTVER}" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  ds-audioid-list.txt file has wrong version"
      fi
    else
      echo "  no ds-audioid-list.txt file"
    fi

    fn=${DATADIR}/profile00/ds-currsong.txt
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

    fn=${DATADIR}/bdjuri.txt
    res=$(($res+1))
    if [[ $fin == T && -f ${fn} ]]; then
      chk=$(($chk+1))
      res=$(($res+1))
      grep "version ${BDJURIVER}" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  incorrect version in bdjuri.txt"
      fi
    else
      echo "  no ${fn}"
    fi

    res=$(($res+1))  # tmp dir
    if [[ $fin == T && -d "${datatop}/tmp" ]]; then
      chk=$(($chk+1))
    else
      echo "  no tmp directory"
    fi

    res=$(($res+1))  # img dir
    if [[ $fin == T && -d "${datatop}/img" ]]; then
      chk=$(($chk+1))
    else
      echo "  no img directory"
    fi

    res=$(($res+1))  # img/profile00 dir
    if [[ $fin == T && -d "${datatop}/img/profile00" ]]; then
      chk=$(($chk+1))
    else
      echo "  no img/profile00 directory"
    fi

    res=$(($res+1))
    if [[ $fin == T && -f "${datatop}/img/profile00/button_add.svg" ]]; then
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
      grep "version ${AUDIOADJVER}" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  audioadjust.txt file has wrong version"
      fi
    else
      echo "  no audioadjust.txt file"
    fi

    res=$(($res+1))  # autoselection.txt file
    fn="${DATADIR}/autoselection.txt"
    if [[ $fin == T && -f "${fn}" ]]; then
      grep "version ${AUTOSELVER}" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  autoselection.txt file has wrong version"
      fi
    else
      echo "  no autoselection.txt file"
    fi

    res=$(($res+1))  # http/ca.crt file
    fn="${HTTPDIR}/ca.crt"
    if [[ $fin == T && -f ${fn} ]]; then
      chk=$(($chk+1))
    else
      echo "  no ca.crt file"
    fi

    res=$(($res+1))  # itunes-fields.txt file
    fn="${DATADIR}/itunes-fields.txt"
    if [[ $fin == T && -f ${fn} ]]; then
      grep "version ${ITUNESFIELDSVER}" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  itunes-fields.txt file has wrong version"
      fi
    else
      echo "  no itunes-fields.txt file"
    fi

    res=$(($res+1))  # sortopt.txt file
    fn="${DATADIR}/sortopt.txt"
    if [[ $fin == T && -f ${fn} ]]; then
      grep "version ${SORTOPTVER}" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  sortopt.txt file has wrong version"
      fi
    else
      echo "  no sortopt.txt file"
    fi

    res=$(($res+1))  # gtk-static.css file
    fn="${DATADIR}/gtk-static.css"
    if [[ $fin == T && -f ${fn} ]]; then
      grep "version ${GTKSTATICVER}" "${fn}" > /dev/null 2>&1
      rc=$?
      if [[ $rc -eq 0 ]]; then
        chk=$(($chk+1))
      else
        echo "  gtk-static.css file has wrong version"
      fi
    else
      echo "  no gtk-static.css file"
    fi

    fna="${DATADIR}/QueueDance.pldances"
    fnb=""
    if [[ $section == de_DE ]]; then
      fna="${DATADIR}/Schlangentanz.pldances"
      fnb="${DATADIR}/QueueDance.pldances"
    fi
    if [[ $section == es_ES ]]; then
      fna="${DATADIR}/Danza en cola.pldances"
      fnb="${DATADIR}/QueueDance.pldances"
    fi
    if [[ $section == fi_FI ]]; then
      fna="${DATADIR}/Jonotanssi.pldances"
      fnb="${DATADIR}/QueueDance.pldances"
    fi
    if [[ $section == fr_FR ]]; then
      fna="${DATADIR}/Danse en file d'attente.pldances"
      fnb="${DATADIR}/QueueDance.pldances"
    fi
    if [[ $section == it_IT ]]; then
      fna="${DATADIR}/Ballo in coda.pldances"
      fnb="${DATADIR}/QueueDance.pldances"
    fi
    if [[ $section == ja_JP ]]; then
      fna="${DATADIR}/キューダンス.pldances"
      fnb="${DATADIR}/QueueDance.pldances"
    fi
    if [[ $section == nl_BE || $section == nl_NL ]]; then
      fna="${DATADIR}/Dans In Wachtrij.pldances"
      fnb="${DATADIR}/QueueDance.pldances"
    fi
    if [[ $section == pl_PL ]]; then
      fna="${DATADIR}/Taniec w kolejce.pldances"
      fnb="${DATADIR}/QueueDance.pldances"
    fi
    if [[ $section == ru_RU ]]; then
      fna="${DATADIR}/Танец в очередь.pldances"
      fnb="${DATADIR}/QueueDance.pldances"
    fi
    res=$(($res+1))  # queuedance.pldances file
    if [[ $fin == T && -f ${fna} ]]; then
      if [[ -f ${fnb} ]]; then
        echo "  extra $(basename ${fnb}) file"
      else
        grep "^# version ${QDANCEVER}" "${fna}" > /dev/null 2>&1
        rc=$?
        if [[ $rc -eq 0 ]]; then
          chk=$(($chk+1))
        else
          echo "  $(basename ${fna}) incorrect version"
        fi
      fi
    else
      echo "  no $(basename ${fna}) file"
    fi

    fna="${DATADIR}/QueueDance.pl"
    fnb=""
    if [[ $section == de_DE ]]; then
      fna="${DATADIR}/Schlangentanz.pl"
      fnb="${DATADIR}/QueueDance.pl"
    fi
    if [[ $section == es_ES ]]; then
      fna="$DATADIR/Danza en cola.pl"
      fnb="${DATADIR}/QueueDance.pl"
    fi
    if [[ $section == fi_FI ]]; then
      fna="${DATADIR}/Jonotanssi.pl"
      fnb="${DATADIR}/QueueDance.pl"
    fi
    if [[ $section == fr_FR ]]; then
      fna="$DATADIR/Danse en file d'attente.pl"
      fnb="${DATADIR}/QueueDance.pl"
    fi
    if [[ $section == it_IT ]]; then
      fna="${DATADIR}/Ballo in coda.pl"
      fnb="${DATADIR}/QueueDance.pl"
    fi
    if [[ $section == ja_JP ]]; then
      fna="${DATADIR}/キューダンス.pl"
      fnb="${DATADIR}/QueueDance.pl"
    fi
    if [[ $section == nl_BE || $section == nl_NL ]]; then
      fna="${DATADIR}/Dans In Wachtrij.pl"
      fnb="${DATADIR}/QueueDance.pl"
    fi
    if [[ $section == pl_PL ]]; then
      fna="${DATADIR}/Taniec w kolejce.pl"
      fnb="${DATADIR}/QueueDance.pl"
    fi
    if [[ $section == ru_RU ]]; then
      fna="${DATADIR}/Танец в очередь.pl"
      fnb="${DATADIR}/QueueDance.pl"
    fi
    res=$(($res+1))  # queuedance.pl file
    if [[ $fin == T && -f ${fna} ]]; then
      if [[ -f ${fnb} ]]; then
        echo "  extra $(basename ${fnb}) file"
      else
        chk=$(($chk+1))
      fi
    else
      echo "  no $(basename ${fna}) file"
    fi

    fn="${DATADIR}/standardrounds.pldances"
    if [[ $section == de_DE ]]; then
      fn="${DATADIR}/Standardrunden.pldances"
    fi
    if [[ $section == es_ES ]]; then
      fn="${DATADIR}/estándares.pldances"
    fi
    if [[ $section == fi_FI ]]; then
      fn="${DATADIR}/vakiokierrokset.pldances"
    fi
    if [[ $section == fr_FR ]]; then
      fn="${DATADIR}/les rounds standards.pldances"
    fi
    if [[ $section == it_IT ]]; then
      fn="${DATADIR}/giri standard.pldances"
    fi
    if [[ $section == ja_JP ]]; then
      fn="${DATADIR}/スタンダードラウンド.pldances"
    fi
    if [[ $section == nl_BE || $section == nl_NL ]]; then
      fn="${DATADIR}/Standaardrondes.pldances"
    fi
    if [[ $section == pl_PL ]]; then
      fn="${DATADIR}/rundy standardowe.pldances"
    fi
    if [[ $section == ru_RU ]]; then
      fn="${DATADIR}/стандартные циклы.pldances"
    fi
    res=$(($res+1))  # standardrounds.pldances file
    if [[ $fin == T && -f ${fn} ]]; then
      grep "^# version ${STDROUNDVER}" "${fn}" > /dev/null 2>&1
      rc=$?
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

    res=$(($res+1))  # data/localization.txt file only in templates
    fn="${DATADIR}/localization.txt"
    if [[ $fin == T && ! -f "${fn}" ]]; then
      chk=$(($chk+1))
    else
      echo "  $(basename ${fn}) exists"
    fi

    res=$(($res+1))  # data/html-list.txt file only in templates
    fn="${DATADIR}/html-list.txt"
    if [[ $fin == T && ! -f "${fn}" ]]; then
      chk=$(($chk+1))
    else
      echo "  $(basename ${fn}) exists"
    fi

    res=$(($res+1))  # data/helpdata.txt file only in templates
    fn="${DATADIR}/helpdata.txt"
    if [[ $fin == T && ! -f "${fn}" ]]; then
      chk=$(($chk+1))
    else
      echo "  $(basename ${fn}) exists"
    fi

    res=$(($res+1))  # mobilemq.html file
    fn="${HTTPDIR}/mobilemq.html"
    if [[ $fin == T && -f ${fn} ]]; then
      grep "<!-- VERSION ${MOBILEMQVER}" "${fn}" > /dev/null 2>&1
      rc=$?
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

    res=$(($res+1))  # bdj4server
    if [[ $fin == T && -f "${target}/bin/bdj4server${sfx}" ]]; then
      chk=$(($chk+1))
    else
      echo "  no bdj4server"
    fi

    # make sure acr_extr gets installed
    res=$(($res+1))
    if [[ $fin == T && -f "${target}/plocal/bin/acr_extr${sfx}" ]]; then
      chk=$(($chk+1))
    else
      echo "  no plocal/bin/acr_extr file"
    fi

    # bdj4 exec
    res=$(($res+1))
    if [[ $fin == T && -f "${target}/bin/bdj4${sfx}" ]]; then
      chk=$(($chk+1))
    else
      echo "  no bdj4 executable"
    fi

    if [[ $datafiles == y ]]; then
      # volume lib
      lvol=$(sed -n -e '/^VOLUME$/ { n; s/^\.\.//; p ; }' "$mconf")
      res=$(($res+1))
      if [[ $fin == T && $libvol == $lvol ]]; then
        chk=$(($chk+1))
      else
        echo "  volume library not set correctly"
      fi

      # pli lib
      lpli=$(sed -n -e '/^PLAYER$/ { n; s/^\.\.//; p ; }' "$mconf")
      res=$(($res+1))
      if [[ $fin == T && $libpli == $lpli ]]; then
        chk=$(($chk+1))
      else
        echo "  pli library not set correctly"
      fi

      # music dir not empty
      mdir=$(sed -n -e '/^DIRMUSIC$/ { n; s/^\.\.//; p ; }' "$mconf")
      res=$(($res+1))
      if [[ $fin == T && $mdir != "" ]]; then
        chk=$(($chk+1))
      else
        echo "  music dir not set correctly"
      fi
    fi
  fi

  if [[ -d "${datatop}" ]]; then
    c=$(ls -1 "${datatop}/asan*" 2>/dev/null | wc -l)
    if [[ $c -ne 0 ]]; then
      echo "ASAN files found"
      exit 1
    fi
  fi

  c=$(ls -1 "${target}/core" 2>/dev/null | wc -l)
  if [[ $c -ne 0 ]]; then
    echo "core file found (tgt)"
    exit 1
  fi
  c=$(ls -1 "${datatop}/core" 2>/dev/null | wc -l)
  if [[ $c -ne 0 ]]; then
    echo "core file found (data)"
    exit 1
  fi
  c=$(ls -1 "${target}/asan*" 2>/dev/null | wc -l)
  if [[ $c -ne 0 ]]; then
    echo "asan files found (tgt)"
    exit 1
  fi
  c=$(ls -1 "${datatop}/asan*" 2>/dev/null | wc -l)
  if [[ $c -ne 0 ]]; then
    echo "asan files found (data)"
    exit 1
  fi

  if [[ $TARGETTOPALTDIR != $target ]]; then
    c=$(ls -1 "${TARGETTOPALTDIR}/core" 2>/dev/null | wc -l)
    if [[ $c -ne 0 ]]; then
      echo "core file found (tgt-alt)"
      exit 1
    fi
  fi
  if [[ $DATATOPALTDIR != $datatop ]]; then
    c=$(ls -1 "${DATATOPALTDIR}/core" 2>/dev/null | wc -l)
    if [[ $c -ne 0 ]]; then
      echo "core file found (data-alt)"
      exit 1
    fi
  fi
  if [[ $TARGETTOPDIR != $target ]]; then
    c=$(ls -1 "${TARGETTOPDIR}/core" 2>/dev/null | wc -l)
    if [[ $c -ne 0 ]]; then
      echo "core file found (tgt-top)"
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
  test -d "$TARGETTOPALTDIR" && rm -rf "$TARGETTOPALTDIR"
  test -d "$DATATOPDIR" && rm -rf "$DATATOPDIR"
  test -d "$DATATOPALTDIR" && rm -rf "$DATATOPALTDIR"
  # linux
  fn="$HOME/.local/share/applications"
  if [[ -d $fn ]]; then
    # there are many .desktop files installed
    rm -f "{$fn}/BDJ4dev*.desktop"
  fi
  # win
  UPROF=$(echo $USERPROFILE | sed -e 's,\\,/,g' -e 's,C:/,/c/,')
  fn="$UPROF/Desktop/BDJ4dev.lnk"
  test -f "$fn" && rm -f "$fn"
  fn="$UPROF/Desktop/BDJ4altdev.lnk"
  test -f "$fn" && rm -f "$fn"
  # macos
  fn="$HOME/Desktop/BDJ4dev.app"
  test -d "$fn" && rm -rf "$fn"
  fn="$HOME/Desktop/BDJ4altdev.app"
  test -d "$fn" && rm -rf "$fn"
  fn="$HOME/Desktop/BDJ4dev"
  test -d "$fn" && rm -rf "$fn"
  fn="$HOME/Desktop/BDJ4altdev"
  test -d "$fn" && rm -rf "$fn"
  fn="$HOME/Applications/BDJ4altdev.app"
  test -d "$fn" && rm -rf "$fn"
  fn="$HOME/Applications/BDJ4dev.app"
  test -d "$fn" && rm -rf "$fn"
  # macos, linux
  fn="$HOME/.config/BDJ4/altinstdirdev.txt"
  test -f "$fn" && rm -f "$fn"
  fn="$HOME/.config/BDJ4/altcountdev.txt"
  test -f "$fn" && rm -f "$fn"
}

function waitForInstallDirRemoval {
  count=0
  while test -d "$UNPACKDIR" && test $count -lt 20; do
    sleep 1
    count=$(($count+1))
  done
  if [[ -d "$UNPACKDIR" ]]; then
    echo "ERR: install-dir removal failed".
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

cleanInstTest

if [[ $readonly == F ]]; then
  # main test db : rebuild of standard test database
  resetInstallDir
  tname=new-install
  echo "== $section $tname"
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
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
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
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
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc u y
  waitForInstallDirRemoval

  # make sure acr_extr gets installed on update
  rm -f $TARGETTOPDIR/plocal/bin/acr_extr

  # update w/various update tasks
  # this should get installed as of version 4.1.0
  resetInstallDir
  tname=update-chk-updater
  echo "== $section $tname"
  checkUpdaterClean $section
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc u y
  waitForInstallDirRemoval
fi

if [[ $readonly == F ]]; then
  # alternate installation
  tname=alt-install
  echo "== $section $tname"
  out=$(cd "$TARGETDIR";./bin/bdj4 --bdj4altinst \
      --verbose --unattended ${quiet} \
      --targetdir "$TARGETTOPALTDIR" \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc n o "${TARGETALTDIR}" "${DATATOPALTDIR}"

  # alternate installation
  tname=alt-install-reinstall
  echo "== $section $tname"
  out=$(cd "$TARGETDIR";./bin/bdj4 --bdj4altinst \
      --verbose --unattended ${quiet} \
      --targetdir "$TARGETTOPALTDIR" \
      --reinstall \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc r o "${TARGETALTDIR}" "${DATATOPALTDIR}"

  # alternate installation
  tname=alt-install-update
  echo "== $section $tname"
  out=$(cd "$TARGETDIR";./bin/bdj4 --bdj4altinst \
      --verbose --unattended ${quiet} \
      --targetdir "$TARGETTOPALTDIR" \
      )
  rc=$?
  checkInstallation $section $tname "$out" $rc u o "${TARGETALTDIR}" "${DATATOPALTDIR}"
fi

if [[ T == T ]]; then
  # install w/o data files
  cleanInstTest
  resetInstallDir
  tname=install-readonly
  echo "== $section $tname"
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
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

# 2024-7-18 not all languages are tested, just a representation.
# nl_NL is important, as there is no nl_NL/ dir.
for section in fi_FI ja_JP nl_BE nl_NL pl_PL ru_RU ; do
  locale=${section}

  cleanInstTest
  resetInstallDir

  # main test db : rebuild of standard test database, nl_BE
  tname=new-install
  echo "== $section $tname"
  out=$(cd "$UNPACKDIRBASE";./bin/bdj4 --bdj4installer \
      --verbose --unattended ${quiet} \
      --targetdir "$TARGETTOPDIR" \
      --unpackdir "$UNPACKDIR" \
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
        --targetdir "$TARGETTOPDIR" \
        --unpackdir "$UNPACKDIR" \
        --locale ${locale} \
        )
    rc=$?
    checkInstallation $section $tname "$out" $rc u y
    waitForInstallDirRemoval
  fi

  if [[ T == T ]]; then
    # alternate installation
    tname=alt-install
    echo "== $section $tname"
    out=$(cd "$TARGETDIR";./bin/bdj4 --bdj4altinst \
        --verbose --unattended ${quiet} \
        --targetdir "$TARGETTOPALTDIR" \
        --locale ${locale} \
        )
    rc=$?
    checkInstallation $section $tname "$out" $rc n o "${TARGETALTDIR}" "${DATATOPALTDIR}"
  fi
done

### end tests

if [[ $keep == F ]]; then
  cleanInstTest
  test -d "$UNPACKDIRSAVE" && rm -rf "$UNPACKDIRSAVE"
fi

echo "tests: $tcount pass: $pass fail: $fail"

exit $grc
