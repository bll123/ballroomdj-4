#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
ver=6

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

xcode-select -p >/dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "Unable to locate MacOS Command Line Tools"
  echo "Attempting to bootstrap install of MacOS Command Line Tools"
  make > /dev/null 2>&1
  echo ""
  echo "After MacOS Command Lines Tools has finished installing,"
  echo "run this script again."
  echo ""
  exit 0
fi

# not sure if there's a way to determine the latest python version
# this needs to be fixed.
pyver=310

skipmpinst=F
case $vers in
  1[456789]*)
    mp_os_nm=$vers
    mp_os_vers=$vers
    echo "This script has no knowledge of this version of MacOS (too new)."
    echo "If macports is already installed, the script can continue."
    echo "Continue? "
    gr=$(getresponse)
    if [[ $gr != Y ]]; then
      exit 1
    fi
    skipmpinst=T
    ;;
  13*)
    mp_os_nm=Ventura
    mp_os_vers=13
    ;;
  12*)
    mp_os_nm=Monterey
    mp_os_vers=12
    ;;
  11*)
    mp_os_nm=BigSur
    mp_os_vers=11
    ;;
  10.15*)
    mp_os_nm=Catalina
    mp_os_vers=10.15
    ;;
  10.14*)
    mp_os_nm=Mojave
    mp_os_vers=10.14
    ;;
  *)
    echo "BallroomDJ 4 cannot be installed on this version of MacOS."
    echo "This version of MacOS is too old."
    echo "MacOS Mojave and later are supported."
    exit 1
    ;;
esac

if [[ $skipmpinst == F ]]; then
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
fi

PATH=$PATH:/opt/local/bin

echo "-- Running MacPorts 'port selfupdate' with sudo"
sudo port selfupdate

echo "-- Running MacPorts 'port upgrade outdated' with sudo"
sudo port upgrade outdated

echo "-- Installing packages needed by BDJ4"
sudo port -N install \
    python${pyver} \
    py${pyver}-pip \
    py${pyver}-wheel \
    curl \
    curl-ca-bundle \
    librsvg \
    gtk3 +quartz \
    adwaita-icon-theme \
    ffmpeg +nonfree -x11
sudo port select --set python python${pyver}
sudo port select --set python3 python${pyver}
sudo port select --set pip pip${pyver}
sudo port select --set pip3 pip${pyver}

echo "-- Cleaning up old MacPorts files"
if [[ -z "$(port -q list inactive)" ]]; then
  sudo port -N reclaim > /dev/null 2>&1 << _HERE_
n
_HERE_
fi

# look for the macos-run-installer script

pattern="macos-run-installer-v[0-9]*.sh"

latest=""
for f in $pattern; do
  if [[ -f $f ]]; then
    chmod a+rx $f
    if [[ $latest == "" ]];then
      latest=$f
    fi
    if [[ $f -nt $latest ]]; then
      latest=$f
    fi
  fi
done

if [[ -f $latest ]]; then
  bash ${latest}
fi

exit 0
