#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

. src/utils/pkgnm.sh

systype=$(uname -s)
case $systype in
  Linux)
    tag=linux
    platform=unix
    ;;
  Darwin)
    tag=macos
    platform=unix
    ;;
  MINGW64*)
    tag=win64
    platform=windows
    echo "sshpass is currently broken in msys2 "
    ;;
  MINGW32*)
    echo "Platform not supported."
    exit 1
    ;;
esac

. ./VERSION.txt
cvers=$(pkgcurrvers)
pnm=$(pkginstnm)
case ${pnm} in
  *-dev)
    echo "Failed: will not push development package."
    exit 1
    ;;
esac

if [[ ! -f ${pnm} ]]; then
  echo "Failed: no release package found."
  exit 1
fi

if [[ $platform != windows ]]; then
  echo -n "sourceforge Password: "
  read -s SSHPASS
  echo ""
  if [[ $SSHPASS == "" ]]; then
    echo "No password."
    exit 1
  fi
  export SSHPASS
fi

if [[ $tag == linux ]]; then
  fn=README.txt
  sed -e "s~#VERSION#~${cvers}~" -e "s~#BUILDDATE#~${BUILDDATE}~" $fn > ${fn}.n
  sshpass -e rsync -v -e ssh ${fn}.n \
      bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/${fn}
  rm -f ${fn}.n

  fn=linux-pre-install
  ver=$(install/${fn}.sh --version)
  sshpass -e rsync -v -e ssh install/${fn}.sh \
      bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/${fn}-v${ver}.sh
  fn=linux-uninstall-bdj4
  sshpass -e rsync -v -e ssh install/${fn}.sh \
      bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/

  server=web.sourceforge.net
  port=22
  project=ballroomdj4
  # ${remuser}@web.sourceforge.net:/home/project-web/${project}/htdocs
  remuser=bll123
  wwwpath=/home/project-web/${project}/htdocs
  ssh="ssh -p $port"
  export ssh

  echo "## updating version file"
  VERFILE=bdj4version.txt
  bd=$BUILDDATE
  cvers=$(pkgcurrvers)
  echo "$cvers ($bd)" > $VERFILE
  for f in $VERFILE; do
    sshpass -e rsync -e "$ssh" -aS \
        $f ${remuser}@${server}:${wwwpath}
  done
  rm -f $VERFILE
fi

if [[ $tag == macos ]]; then
  fn=macos-pre-install-macports
  ver=$(install/${fn}.sh --version)
  sshpass -e rsync -v -e ssh install/${fn}.sh \
      bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/${fn}-v${ver}.sh

  fn=macos-run-installer
  ver=$(install/${fn}.sh --version)
  sshpass -e rsync -v -e ssh install/${fn}.sh \
      bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/${fn}-v${ver}.sh

  fn=macos-uninstall-bdj4
  ver=$(install/${fn}.sh --version)
  sshpass -e rsync -v -e ssh install/${fn}.sh \
      bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/${fn}-v${ver}.sh
fi

if [[ $platform != windows ]]; then
  sshpass -e rsync -v -e ssh ${pnm} \
    bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/
else
  rsync -v -e ssh ${pnm} \
    bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/
fi

exit 0
