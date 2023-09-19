#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
ver=2

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

cdir=${XDG_CACHE_HOME:-$HOME/.cache}
cachedir="${cdir}/BDJ4"

cdir=${XDG_CONFIG_HOME:-$HOME/.config}
confdira="${HOME}/.config/BDJ4"
confdirb="${cdir}/BDJ4"
instdir="${cdir}/BDJ4/installdir.txt"
altinstdir="${cdir}/BDJ4/altinstdir.txt"
appdir=$HOME/.local/share/applications
desktop=$(xdg-user-dir DESKTOP)

echo "Uninstall the BallroomDJ 4 Application? "
gr=$(getresponse)
if [[ $gr == Y ]]; then
  dir="$HOME/BDJ4"
  for fn in $instdir $altinstdir; do
    dir=""
    if [[ -f $instdir ]]; then
      dir=$(cat $instdir)
    fi
    if [[ $dir != "" ]]; then
      test -d ${dir} && rm -rf ${dir}
    fi
  fi
  test -d ${confdir} && rm -rf ${confdir}
  test -d ${cachedir} && rm -rf ${cachedir}
  test -f ${desktop}/BDJ4.desktop && rm -f ${desktop}/BDJ4.desktop
  test -f ${desktop}/bdj4.desktop && rm -f ${desktop}/bdj4.desktop
  test -f ${appdir}/BDJ4.desktop && rm -f ${appdir}/BDJ4.desktop
  # remove any old mutagen installed for the user
  pipp=/usr/bin/pip
  if [[ -f /usr/bin/pip3 ]]; then
    pipp=/usr/bin/pip3
  fi
  ${pipp} uninstall -y mutagen > /dev/null 2>&1
  ${pipp} uninstall -y --break-system-packages mutagen > /dev/null 2>&1
  echo "-- BDJ4 application removed."
fi

exit 0
