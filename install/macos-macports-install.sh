#!/bin/bash

if [[ ! -f /usr/bin/sw_vers ]]; then
  echo "Not running on Mac OS"
  exit 1
fi

if [[ ! -d /Library/Developer/CommandLineTools ]]; then
  echo "Unable to locate Command Line Tools"
  echo ""
  echo "For Mac OS, either XCode (40GB) or the 'Command Line Tools' (2GB) "
  echo "must be installed."
  echo ""
  echo "An apple developer account is necessary to download the "
  echo "'Command Line Tools'."
  echo ""
  echo "Visit this link:"
  echo "https://developer.apple.com/download/all/?q=command%20line%20tools"
  echo ""
  echo "Unfortunately, the description no longer indicates which version"
  echo "of Mac OS the 'Command Line Tools' are intended for."
  echo "  Monterey: Command Line Tools 13.2"
  echo "  Big Sur: Command Line Tools 13.2"
  echo "  Catalina: Command Line Tools 12.4"
  echo "  Mojave: Command Line Tools 10.1"
  echo "  High Sierra: Command Line Tools 9.4.1"
  echo ""
  exit 1
fi

vers=$(sw_vers -productVersion)
# not sure if there's a way to determine the latest python version
pyver=310

case $vers in
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
esac

mp_installed=F
if [[ -d /opt/local && -d /opt/local/share/macports && -f /opt/local/bin/port ]]; then
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
sudo port install flac sox vorbis-tools
#sudo port install ffmpeg +nonfree -x11
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
