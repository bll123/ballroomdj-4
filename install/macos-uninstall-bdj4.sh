#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
ver=7

if [[ $1 == --version ]]; then
  echo ${ver}
  exit 0
fi

function getresponse () {
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
  if [[ -f ${loc} ]]; then
    dir=$(cat "$loc")
    if [[ $dir != "" && -d $dir ]]; then
      rm -rf "$dir"
      nm=$(basename "$dir" | sed 's,\.app,,')
      ddir="$HOME/Library/Application Support/${nm}"
      if [[ $nm != "" && -d $ddir ]]; then
        rm -rf "$ddir"
      fi
      # shortcuts on desktop
      sfn="$HOME/Desktop/${nm}"
      test -h "$sfn" && rm -f "$sfn"
      sfn="$HOME/Desktop/${nm}.app"
      test -h "$sfn" && rm -f "$sfn"
    fi
  fi
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

# config dir
cdir=${XDG_CONFIG_HOME:-$HOME/.config}
confdir="${cdir}/BDJ4"
cdir=${XDG_CACHE_HOME:-$HOME/.cache}
cachedir="${cdir}/BDJ4"
cdir="$HOME/.local/share"
icondir="${cdir}/icons/hicolor/scalable/apps"

echo "Uninstall the BallroomDJ 4 Application and Data? "
gr=$(getresponse)
if [[ $gr == Y ]]; then
  # config dir
  instloc="${confdir}/installdir.txt"

  doremove "$instloc"

  for altidx in 01 02 03 04 05 06 07 08 09; do
    fn="${confdirb}/altinstdir${altidx}.txt"
    doremove "$fn"
  done

  dir="$HOME/Applications/BDJ4.app"
  test -d "${dir}" && rm -rf "${dir}"
  ddir="$HOME/Library/Application Support/BDJ4"
  test -d "$ddir" && rm -rf "$ddir"

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
  crontab -l | sed -e '/bdj4cleantmp/ d' > $TMP
  crontab $TMP
  rm -f $TMP

  # cache dir
  test -d "$cachedir" && rm -rf "$cachedir"
  test -d "$confdir" && rm -rf "$confdir"
  rm -f ${icondir}/bdj4_icon*.svg

  echo "-- BDJ4 application removed."
fi

if [[ -d /opt/local/bin ]]; then
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
fi

if [[ -d /opt/pkg/bin ]]; then
  echo "Uninstall pkgsrc? "
  gr=$(getresponse)
  if [[ $gr == Y ]]; then
    sudo rm -f /opt/pkg
    sudo rm -f /etc/paths.d/pkgsrc
  fi
fi

if [[ -d /opt/homebrew/bin ]]; then
  echo "Uninstall Homebrew? "
  gr=$(getresponse)
  if [[ $gr == Y ]]; then
    NONINTERACTIVE=1 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/uninstall.sh)"
    sudo rm -f /opt/homebrew
    sudo rm -f /etc/paths.d/homebrew
  fi
fi

sudo -k

exit 0
