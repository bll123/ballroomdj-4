#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
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
    ;;
  MINGW32*)
    echo "Platform not supported."
    exit 1
    ;;
esac

if [[ $SSHPASS == "" ]]; then
  echo -n "sourceforge Password: "
  read -s SSHPASS
  echo ""
  export SSHPASS
fi

if [[ $tag == linux ]]; then
  sshpass -e rsync -aS --delete -e ssh doxygen \
      bll123@web.sourceforge.net:/home/project-web/ballroomdj4/htdocs
fi

exit 0
