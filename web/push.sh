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
case $systype in
  Linux)
    tag=linux
    platform=unix
    sfx=
    ;;
  Darwin)
    tag=macos
    platform=unix
    sfx=
    ;;
  MINGW64*)
    tag=win64
    platform=windows
    sfx=.exe
    echo "sshpass is currently broken in msys2 "
    ;;
  MINGW32*)
    tag=win32
    platform=windows
    sfx=.exe
    echo "sshpass is currently broken in msys2 "
    ;;
esac

if [[ $platform != windows ]]; then
  echo -n "sourceforge Password: "
  read -s SSHPASS
  echo ""
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

if [[ ! -f bdj4-${VERSION}-installer-${tag}${datetag}${rlstag}${sfx} ]]; then
  echo "Failed: no release package found."
  exit 1
fi

if [[ $platform != windows ]]; then
  sshpass -e rsync -v -e ssh bdj4-${VERSION}-installer-${tag}${datetag}${rlstag}${sfx} \
    bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/
else
  rsync -v -e ssh bdj4-${VERSION}-installer-${tag}${datetag}${rlstag}${sfx} \
    bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/
fi

if [[ $tag == macos ]]; then
  sshpass -e rsync -v -e ssh install/macos-pre-install.sh \
      bll123@frs.sourceforge.net:/home/frs/project/ballroomdj4/
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
