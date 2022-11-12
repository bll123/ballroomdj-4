#!/bin/bash
#

cwd=$(pwd)
case $cwd in
  */bdj4)
    ;;
  */bdj4/*)
    cwd=$(dirname $cwd)
    while : ; do
      case $cwd in
        */bdj4)
          break
          ;;
      esac
      cwd=$(dirname $cwd)
    done
    cd $cwd
    ;;
esac

systype=$(uname -s)
arch=$(uname -m)
case $systype in
  Linux)
    tag=linux
    platform=unix
    sfx=
    archtag=
    ;;
  Darwin)
    tag=macos
    platform=unix
    sfx=
    case $arch in
      x86_64)
        archtag=-intel
        ;;
      arm64)
        archtag=-m1
        ;;
    esac
    ;;
  MINGW64*)
    tag=win64
    platform=windows
    sfx=.exe
    archtag=
    echo "sshpass is currently broken in msys2 "
    ;;
  MINGW32*)
    tag=win32
    platform=windows
    sfx=.exe
    archtag=
    echo "sshpass is currently broken in msys2 "
    ;;
esac

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

. ./VERSION.txt

case $RELEASELEVEL in
  alpha|beta)
    rlstag=-$RELEASELEVEL
    ;;
  production)
    rlstag=""
    ;;
esac

datetag=""
if [[ $rlstag != "" ]]; then
  datetag=-$BUILDDATE
fi

pnm=bdj4-${VERSION}-installer-${tag}${archtag}${datetag}${rlstag}${sfx}

if [[ ! -f ${pnm} ]]; then
  echo "Failed: no release package found."
  exit 1
fi

if [[ $platform != windows ]]; then
  sshpass -e rsync -v -e ssh ${pnm} \
    bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/
else
  rsync -v -e ssh ${pnm} \
    bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/
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
fi

if [[ $tag == linux ]]; then
  sshpass -e rsync -v -e ssh README.txt \
      bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/
  sshpass -e rsync -v -e ssh install/linux-pre-install.sh \
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
  . ./VERSION.txt
  if [[ $RELEASELEVEL != "" ]]; then
    bd=$BUILDDATE
  fi
  echo "$VERSION $bd $RELEASELEVEL" > $VERFILE
  for f in $VERFILE; do
    sshpass -e rsync -e "$ssh" -aS \
        $f ${remuser}@${server}:${wwwpath}
  done
  rm -f $VERFILE
fi

exit 0
