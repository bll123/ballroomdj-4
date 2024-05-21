#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
ver=19

if [[ $1 == --version ]]; then
  echo ${ver}
  exit 0
fi

function getresponse {
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

skipmpinst=F
#  when adding a new version, be sure to update the 'too-new' regex.    ###
case $vers in
  1[56789]*)
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
  14*)
    mp_os_nm=Sonoma
    mp_os_vers=14
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
  else
    echo "-- MacPorts is not installed"
  fi

  if [[ $mp_installed == T ]]; then
    port version > /dev/null 2>&1
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "-- MacPorts needs an upgrade"
      mp_installed=F
    fi
  fi

  if [[ $mp_installed == F ]]; then
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

sudo -v

echo "-- Running MacPorts 'port selfupdate' with sudo"
sudo port selfupdate

sudo -v

TLOC=${TMPDIR:-/var/tmp}
TFN="$TLOC/bdj4-pre-inst.tmp"
VARIANTSCONF=/opt/local/etc/macports/variants.conf

grep -- "^.x11 .no_x11 .quartz" $VARIANTSCONF > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "-- Updating variants.conf"
  cp $VARIANTSCONF "$TFN"
  echo "-x11 +no_x11 +quartz" >> "$TFN"
  sudo cp "$TFN" $VARIANTSCONF
  rm -f "$TFN"
fi

echo "-- Running MacPorts 'port upgrade outdated' with sudo"
sudo port upgrade outdated

sudo -v

# remove any old user-install mutagen
pipp=/opt/local/bin/pip
if [[ -f /opt/local/bin/pip3 ]]; then
  pipp=/opt/local/bin/pip3
fi
${pipp} uninstall -y mutagen > /dev/null 2>&1

echo "-- Installing packages needed by BDJ4"
# using our own icu and libid3tag
sudo port -N install \
    mpstats \
    libxml2 \
    glib2 +quartz \
    json-c \
    curl \
    curl-ca-bundle \
    libogg \
    libopus \
    libvorbis \
    opusfile \
    flac \
    libgcrypt \
    librsvg \
    gtk3 +quartz \
    adwaita-icon-theme \
    ffmpeg +nonfree -x11 -rav1e \
    chromaprint

sudo -v

echo "-- Cleaning up old MacPorts files"

sudo port -N uninstall \
    taglib \
    py310-mutagen \
    py311-mutagen \
    py312-mutagen

sudo -v

if [[ -z "$(port -q list inactive)" ]]; then
  sudo port reclaim --disable-reminders
  sudo port -N reclaim
fi

sudo -k

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
