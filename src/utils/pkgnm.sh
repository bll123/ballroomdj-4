#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

pn_spkgnm=bdj4
pn_tag=""
pn_rlstag=""
pn_devtag=""
pn_datetag=""
pn_archtag=""
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
  case ${pn_systype} in
    Linux)
      pn_tag=linux
      pn_sfx=
      pn_archtag=
      dver=$(cat /etc/debian_version)
      case ${dver} in
        11*)
          pn_tag=linux-older
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
      ;;
    MINGW32*)
      echo "Platform not supported"
      exit 1
      ;;
  esac
}

function pkgsrcnm {
  pkgnmgetdata
  nm=${pn_spkgnm}-src-${VERSION}${pn_rlstag}${pn_devtag}.tar.gz
  echo $nm
}

function pkginstnm {
  pkgnmgetdata
  nm=${pn_spkgnm}-installer-${pn_tag}${pn_archtag}-${VERSION}${pn_rlstag}${pn_devtag}${pn_datetag}${pn_sfx}
  echo $nm
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

