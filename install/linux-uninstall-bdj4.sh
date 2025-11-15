#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
ver=5

TMP=/tmp/bdj4-tmp.txt

if [[ $1 == --version ]]; then
  echo ${ver}
  exit 0
fi

getresponse () {
  echo -n "[y/N]: " > /dev/tty
  read answer
  case $answer in
    Y|y|yes|Yes|YES)
      answer=Y
      ;;
    *)
      answer=N
      ;;
  esac
  echo $answer
}

function doremove () {
  loc=$1

  dir=""
  if [[ -f $loc ]]; then
    dir=$(cat "$loc")
    if [[ ${dir} != "" && -d ${dir} ]]; then
      rm -rf "${dir}"
    fi
  fi
}


if [[ $(uname -s) != Linux ]]; then
  echo "Not running on Linux"
  exit 1
fi

cdir=${XDG_CACHE_HOME:-$HOME/.cache}
cachedir="${cdir}/BDJ4"

cdir=${XDG_CONFIG_HOME:-$HOME/.config}
confdira="${HOME}/.config/BDJ4"         # old
confdirb="${cdir}/BDJ4"
instloc="${confdirb}/installdir.txt"
altinstloc="${confdirb}/altinstdir.txt"
cdir="$HOME/.local/share"
appdir="${cdir}/applications"
icondir="${cdir}/icons/hicolor/scalable/apps"
desktop=$(xdg-user-dir DESKTOP)

echo "Uninstall the BallroomDJ 4 Application? "
gr=$(getresponse)
if [[ $gr == Y ]]; then
  doremove "$instloc"

  for altidx in 01 02 03 04 05 06 07 08 09; do
    fn="${confdirb}/altinstdir${altidx}.txt"
    doremove "$fn"
  done

  if [[ -d $HOME/BDJ4 && -d $HOME/BDJ4/data && -d $HOME/BDJ4/bin/bdj4 ]]; then
    rm -rf $HOME/BDJ4
  fi

  rm -f "${desktop}"/BDJ4*.desktop
  rm -f "${desktop}"/bdj4.desktop  # old
  rm -f "${appdir}"/BDJ4*.desktop
  rm -f "${icondir}"/bdj4_icon*.svg

  test -d "${confdira}" && rm -rf "${confdira}"  # old
  test -d "${confdirb}" && rm -rf "${confdirb}"
  test -d "${cachedir}" && rm -rf "${cachedir}"

  # crontab
  crontab -l | sed -e '/bdj4cleantmp/ d' > $TMP
  crontab $TMP
  rm -f $TMP

  echo "-- BDJ4 application removed."
fi

exit 0
