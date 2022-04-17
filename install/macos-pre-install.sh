#!/bin/bash

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
echo "This script uses the 'sudo' command to run various commands"
echo "in a privileged state.  You will be required to enter your"
echo "password."
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

vers=$(sw_vers -productVersion)

cltinstpath=/tmp/.com.apple.dt.CommandLineTools.installondemand.in-progress
xcode-select -p >/dev/null
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "Unable to locate MacOS Command Line Tools"
  echo "Setting up to install MacOS Command Line Tools"
  echo "Please wait..."
  touch $cltinstpath
  softwareupdate -l >/dev/null 2>&1
  echo ""
  echo "Now run Software Update (from Settings)"
  echo "and install the Command Lines Tools update."
  ehco ""
  echo "After installation, run this script again."
  echo ""
  echo "Software Update will continue to list the"
  echo "Command Lines Tools package as outdated."
  echo "This will be fixed when this script is re-run."
  echo ""
  exit 0
fi
test -f $cltinstpath && rm -f $cltinstpath

# not sure if there's a way to determine the latest python version
# this needs to be fixed
pyver=310

case $vers in
  1[3456789]*)
    mp_os_nm=$vers
    mp_os_vers=$vers
    # this will fail to install macports.
    # the script needs to be updated for a new version.
    ;;
  12*)
    mp_os_nm=Monterey
    mp_os_vers=12
    ;;
  11*)
    mp_os_nm="BigSur"
    mp_os_vers=11
    ;;
  10.15*)
    mp_os_nm="Catalina"
    mp_os_vers=10.15
    ;;
  10.14*)
    mp_os_nm="Mojave"
    mp_os_vers=10.14
    ;;
  10.13*)
    mp_os_nm="HighSierra"
    mp_os_vers=10.13
    ;;
  *)
    echo "BallroomDJ 4 cannot be installed on this version of MacOS."
    echo "This version of MacOS is too old."
    exit 1
    ;;
esac

mp_installed=F
if [[ -d /opt/local && \
    -d /opt/local/share/macports && \
    -f /opt/local/bin/port ]]; then
  mp_installed=T
fi

if [[ $mp_installed == F ]]; then
  echo "-- MacPorts is not installed"

  url=https://github.com/macports/macports-base/releases
  # find the current version
  mp_tag=$(curl --include --head --silent \
    ${url}/latest |
    grep '^.ocation:' |
    sed -e 's,.*/,,' -e 's,\r$,,')
  mp_vers=$(echo ${mp_tag} | sed -e 's,^v,,')

  url=https://github.com/macports/macports-base/releases/download
  pkgnm=MacPorts-${mp_vers}-${mp_os_vers}-${mp_os_nm}.pkg
  echo "-- Downloading MacPorts"
  curl -JOL ${url}/${mp_tag}/${pkgnm}
  echo "-- Installing MacPorts using sudo"
  sudo installer -pkg ${pkgnm} -target /Applications
  rm -f ${pkgnm}
fi

echo "-- Running MacPorts 'port selfupdate' with sudo"
sudo port selfupdate
echo "-- Running MacPorts 'port upgrade outdated' with sudo"
sudo port upgrade outdated

echo "-- Installing packages needed by BDJ4"
sudo port install gtk3 +quartz -x11
sudo port install ffmpeg +nonfree -x11
sudo port install python${pyver} py${pyver}-pip py${pyver}-wheel
sudo port select --set python python310
sudo port select --set python3 python310
sudo port select --set pip py310-pip
sudo port select --set pip3 py310-pip

echo "-- Cleaning up old MacPorts files"
if [[ -z "$(port -q list inactive)" ]]; then
  sudo port -N reclaim > /dev/null 2>&1 << _HERE_
n
_HERE_
fi

exit 0
