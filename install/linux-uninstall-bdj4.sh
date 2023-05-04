#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
ver=1

if [[ $1 == --version ]]; then
  echo ${ver}
  exit 0
fi

getresponse () {
  echo -n "[Y/n]: " > /dev/tty
  read answer
  case $answer in
    Y|y|yes|Yes|YES|"")
      answer=Y
      ;;
    *)
      answer=N
      ;;
  esac
  echo $answer
}

if [[ $(uname -s) != Linux ]]; then
  echo "Not running on Linux"
  exit 1
fi

confdir=$HOME/.config/BDJ4
instdir=$HOME/.config/BDJ4/installdir.txt
appdir=$HOME/.local/share/applications
desktop=$(xdg-user-dir DESKTOP)

echo "Uninstall the BallroomDJ 4 Application? "
gr=$(getresponse)
if [[ $gr == Y ]]; then
  dir="$HOME/BDJ4"
  if [[ -f $instdir ]]; then
    dir=$(cat $instdir)
  fi
  if [[ $dir != "" ]]; then
    test -d ${dir} && rm -rf ${dir}
  fi
  test -d ${confdir} && rm -rf ${confdir}
  test -f ${desktop}/BDJ4.desktop && rm -f ${desktop}/BDJ4.desktop
  test -f ${desktop}/bdj4.desktop && rm -f ${desktop}/bdj4.desktop
  test -f ${appdir}/BDJ4.desktop && rm -f ${appdir}/BDJ4.desktop
  echo "-- BDJ4 application removed."
fi

exit 0
