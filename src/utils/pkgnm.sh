#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

pn_spkgnm=bdj4
pn_tag=""
pn_rlstag=""
pn_devtag=""
pn_datetag=""
pn_archtag=""
pn_date=$(date '+%Y%m%d')
BUILD=""
BUILDDATE=""
RELEASELEVEL=""
DEVELOPMENT=""
VERSION=""

function pkgnmgetdata {
  . ./VERSION.txt

  case $RELEASELEVEL in
    alpha|beta)
      pn_rlstag=-$RELEASELEVEL
      ;;
    production)
      pn_rlstag=""
      ;;
  esac

  case $DEVELOPMENT in
    "")
      ;;
    *)
      pn_devtag=-$DEVELOPMENT
      ;;
  esac

  pn_datetag=""
  if [[ $RELEASELEVEL == alpha ]]; then
    pn_datetag=-$BUILDDATE
  fi

  pn_systype=$(uname -s)
  pn_arch=$(uname -m)
  pn_dist=""
  case ${pn_systype} in
    Linux)
      pn_tag=linux
      pn_dist=-unknown
      pn_sfx=
      pn_archtag=
      osid=$(grep '^ID=' /etc/os-release | sed 's/^ID=//')
      case ${osid} in
        fedora)
          pn_dist=-fedora
          ;;
        *opensuse*)
          tver=$(grep '^VERSION_ID=' /etc/os-release | sed 's,VERSION_ID=,,')
          tver=$(echo ${tver} | sed 's,\..*,,;s,",,g')
          pn_dist=-opensuse${tver}
          ;;
        manjaro*)
          pn_dist=-arch
          ;;
        debian)
          # debian version is numeric on MX Linux and Debian
          # other debian based linuxes may use names.
          dver=$(cat /etc/debian_version)
          dver=$(echo ${dver} | sed 's,\..*,,')
          pn_dist=-debian${dver}
          ;;
      esac
      ;;
    Darwin)
      pn_tag=macos
      pn_sfx=
      case $pn_arch in
        x86_64)
          pn_archtag=-intel
          ;;
        arm64)
          pn_archtag=-applesilicon
          ;;
      esac
      ;;
    MINGW64*)
      pn_tag=win64
      pn_sfx=.exe
      pn_archtag=
      case ${MSYSTEM} in
        MINGW64)
          ;;
        UCRT64)
          ;;
        *)
          echo "Environment not supported"
          exit 1
          ;;
      esac
      ;;
    MINGW32*)
      echo "Platform not supported"
      exit 1
      ;;
  esac
}

function pkgsrcnm {
  pkgnmgetdata
  nm=${pn_spkgnm}-src-${VERSION}${pn_rlstag}-${pn_date}${pn_devtag}
  echo $nm
}

function pkgsrcadditionalnm {
  pkgnmgetdata
  ext=.tar.gz
  if [[ ${pn_tag} == win64 ]]; then
    ext=.zip
  fi
  nm=${pn_spkgnm}-src-${pn_tag}${pn_rlstag}${pn_devtag}-${pn_date}${ext}
  echo $nm
}

function pkginstnm {
  pkgnmgetdata
  nm=${pn_spkgnm}-installer-${pn_tag}${pn_dist}${pn_archtag}-${VERSION}${pn_rlstag}${pn_devtag}${pn_datetag}${pn_sfx}
  echo $nm
}

function pkgbasevers {
  pkgnmgetdata
  basevers=${VERSION}${pn_rlstag}${pn_devtag}
  echo $basevers
}

function pkgcurrvers {
  pkgnmgetdata
  currvers=${VERSION}${pn_rlstag}${pn_devtag}${pn_datetag}
  echo $currvers
}

function pkglongvers {
  pkgnmgetdata
  currvers=${VERSION}${pn_rlstag}${pn_devtag}-${BUILDDATE}-${BUILD}
  echo $currvers
}

function pkgwebvers {
  pkgnmgetdata
  currvers="${VERSION} (${BUILDDATE})"
  echo $currvers
}

