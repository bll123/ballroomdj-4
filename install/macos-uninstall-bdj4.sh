#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
ver=5

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

if [[ $(uname -s) != Darwin ]]; then
  echo "Not running on MacOS"
  exit 1
fi

echo ""
echo "This script uses the 'sudo' command to uninstall MacPorts"
echo "You will be required to enter your password."
echo ""
echo "For security reasons, this script should be reviewed to"
echo "determine that your password is not mis-used and no malware"
echo "is installed."
echo ""
echo "Continue? "
gr=$(getresponse)
if [[ $gr != Y ]]; then
  exit 1
fi

TMP=/tmp/bdj4-ct.txt

# remove any old mutagen installed for the user
pip3 uninstall -y mutagen > /dev/null 2>&1

# config dir
cdir=${XDG_CONFIG_HOME:-$HOME/.config}
confdir="${cdir}/BDJ4"

echo "Uninstall the BallroomDJ 4 Application and Data? "
gr=$(getresponse)
if [[ $gr == Y ]]; then
  # cache dir
  cdir=${XDG_CACHE_HOME:-$HOME/.cache}
  cachedir="${cdir}/BDJ4"
  test -d "$cachedir" && rm -rf "$cachedir"
  # config dir
  instloc="${confdirb}/instdir.txt"
  altinstloc="${confdirb}/altinstdir.txt"
  for fn in "$instloc" "$altinstloc"; do
    dir=""
    if [[ -f ${fn} ]]; then
      dir=$(cat "$fn")
      if [[ $dir != "" ]]; then
        test -d "$dir" && rm -rf "$dir"
        nm=$(basename "$dir" | sed 's,\.app,,')
        ddir="$HOME/Library/Application Support/${nm}"
        if [[ $nm != "" ]]; then
          test -d "$ddir" && rm -rf "$ddir"
        fi
        # shortcuts on desktop
        sfn="$HOME/Desktop/${nm}"
        test -h "$sfn" && rm -f "$sfn"
        sfn="$HOME/Desktop/${nm}.app"
        test -h "$sfn" && rm -f "$sfn"
      fi
    fi
    dir="$HOME/Applications/BDJ4.app"
    test -d "${dir}" && rm -rf "${dir}"
    ddir="$HOME/Library/Application Support/BDJ4"
    test -d "$ddir" && rm -rf "$ddir"
  done

  # installed themes
  for fn in "$HOME/.themes/macOS-Mojave-dark" \
      "$HOME/.themes/macOS-Mojave-light" \
      "$HOME/.themes/Mojave-dark-solid" \
      "$HOME/.themes/Mojave-light-solid" \
      ; do
    test -h "$fn" && rm -f "$fn"
  done
  # It is possible the user has other GTK stuff installed and could
  # have other themes.  Try to remove the dir, but don't worry if it fails.
  dir="$HOME/.themes"
  test -d "$dir" && rmdir "$dir" > /dev/null 2>&1

  # crontab
  crontab -l | sed -e '/BDJ4/ d' > $TMP
  crontab $TMP
  rm -f $TMP

  test -d "$confdir" && rm -rf "$confdir"

  echo "-- BDJ4 application removed."
fi

echo "Uninstall MacPorts? "
gr=$(getresponse)
if [[ $gr == Y ]]; then
  TMP=tmp-umacports.txt

  sudo -v

  echo "-- getting ports list"
  sudo port list installed > $TMP
  sudo -v
  echo "-- uninstalling all ports"
  sudo port -N uninstall --follow-dependents $(cat $TMP)
  sudo -v
  echo "-- cleaning all ports"
  sudo port -N clean --all $(cat $TMP)
  sudo -v
  rm -f ${TMP} > /dev/null 2>&1
  for d in \
      /opt/local \
      /Applications/DarwinPorts \
      /Applications/MacPorts \
      /Library/LaunchDaemons/org.macports.* \
      /Library/Receipts/DarwinPorts*.pkg \
      /Library/Receipts/MacPorts*.pkg \
      /Library/StartupItems/DarwinPortsStartup \
      /Library/Tcl/darwinports1.0 \
      /Library/Tcl/macports1.0 \
      ~/.macports \
      ; do
    echo "-- removing ${d}"
    sudo rm -rf ${d}
  done
fi

sudo -k

exit 0
