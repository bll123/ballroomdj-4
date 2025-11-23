#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
ver=24

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
  echo "use System Settings / General / Software Update"
  echo "to check for updates to the Command Line Tools."
  echo "then run this script again."
  echo ""
  exit 0
fi

skipinst=F
oldmacos=F

case $vers in
  10.*)
    # in version 10, the macos release name changes
    # were based on the second value.
    os_vers=$(echo $vers | sed 's,^10\.\([0-9]*\).*,10.\1,')
    ;;
  *)
    os_vers=$(echo $vers | sed 's,\..*,,')
    ;;
esac

case $os_vers in
  2[6-9])
    # 26: tahoe
    ;;
  1[1-5])
    # 11: big sur, 12: monterey, 13: ventura, 14: sonoma, 15: sequoia
    ;;
  *)
    # 10.12: sierra, 10.13: high sierra, 10.14: mojave, 10.15: catalina
    oldmacos=T
    ;;
esac

if [[ $oldmacos == T ]]; then
  echo "BallroomDJ 4 cannot be installed on this version of MacOS."
  echo "This version of MacOS is too old."
  echo "MacOS Big Sur and later are supported."
  echo "Contact BDJ4 support."
  exit 1
fi

arch=$(uname -m)

if [[ $skipinst == F ]]; then
  pkgsrc_installed=F
  if [[ -d /opt/pkg ]]; then
    pfx=/opt/pkg
  fi
  if [[ -d $pfx && -f ${pfx}/bin/pkgin ]]; then
    pkgsrc_installed=T
  else
    echo "-- pkgsrc is not installed"
  fi

  if [[ $pkgsrc_installed == F ]]; then
    if [[ $arch == arm64 ]]; then
      BOOTSTRAP_TAR="bootstrap-macos14.5-trunk-arm64-20251114.tar.gz"
      BOOTSTRAP_SHA="512a3c209fa0e83b4f614bc6ce11928c10b75cd0"
    elif [[ $arch == x86_64 ]]; then
      BOOTSTRAP_TAR="bootstrap-macos12.3-trunk-x86_64-20240426.tar.gz"
      BOOTSTRAP_SHA="2b151d12576befd85312ddb5261307998f16df2f"
    else
      echo "-- Unknown architecture."
      exit 1
    fi
    curl -O https://pkgsrc.smartos.org/packages/Darwin/bootstrap/${BOOTSTRAP_TAR}
    sudo -v
    echo "${BOOTSTRAP_SHA} ${BOOTSTRAP_TAR}" | shasum -c -
    rc=$?
    if [[ $rc != 0 ]]; then
      echo "-- Download of pkgsrc failed."
      exit 1
    fi
    sudo tar -zxpf ${BOOTSTRAP_TAR} -C /
    rm -f ${BOOTSTRAP_TAR}
  fi
fi

PATH=$PATH:/opt/pkg/bin

sudo -v

TLOC=${TMPDIR:-/var/tmp}
TFN="$TLOC/bdj4-pre-inst.tmp"

echo "-- Running pkgsrc update with sudo"
sudo pkgin -y update
sudo -v
sudo pkgin -y upgrade
sudo -v

echo "-- Installing packages needed by BDJ4"
# using our own libid3tag
# the librsvg-2.60 default does not work.
sudo pkgin -y install \
    gettext \
    icu \
    curl \
    flac \
    json-c \
    libgcrypt \
    libogg \
    libopus \
    libvorbis \
    opusfile \
    libxml2 \
    xorgproto \
    librsvg-2.40.21nb28 \
    adwaita-icon-theme \
    glib2 \
    gtk3+

sudo -v

if [[ $arch == arm64 ]]; then
  sudo pkgin -y install chromaprint
fi

sudo -v
sudo pkgin -y update
sudo -v
sudo pkgin -y upgrade
sudo -v

# do not override macos getopt
if [[ -f /opt/pkg/include/getopt.h ]]; then
  sudo mv /opt/pkg/include/getopt.h /opt/pkg/include/getopt.h.ng
fi

sudo pkgin -y autoremove

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
