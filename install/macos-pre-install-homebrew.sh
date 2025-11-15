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

if [[ true ]]; then
  echo "Homebrew cannot be used on MacOS."
  echo "a) On Intel MacOS, Homebrew refuses to make necessary libraries available."
  echo "b) On Apple silicon MacOS, BDJ4+Homebrew does not work properly."
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

skipmpinst=F
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

if [[ -d /usr/local/Homebrew ]]; then
  echo "Homebrew cannot be used on intel MacOS."
  echo "Homebrew refuses to make necessary libraries available for use."
  exit 1
fi

if [[ $skipmpinst == F ]]; then
  brew_installed=F
  if [[ -d /opt/homebrew ]]; then
    pfx=/opt/homebrew
  fi
  if [[ -d /usr/local/Homebrew ]]; then
    pfx=/usr/local
  fi
  if [[ -d $pfx && \
      -d ${pfx}/Cellar && \
      -f ${pfx}/bin/brew ]]; then
    brew_installed=T
  else
    echo "-- HomeBrew is not installed"
  fi

  if [[ $brew_installed == F ]]; then
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  fi
fi

if [[ -d /opt/homebrew ]]; then
  pfx=/opt/homebrew
fi
if [[ -d /usr/local/Homebrew ]]; then
  pfx=/usr/local
fi
PATH=$PATH:${pfx}/bin

sudo -v

TLOC=${TMPDIR:-/var/tmp}
TFN="$TLOC/bdj4-pre-inst.tmp"

echo "-- Running HomeBrew 'update/upgrade' with sudo"
brew update && brew outdated && brew upgrade && brew cleanup

sudo -v

echo "-- Installing packages needed by BDJ4"
# using our own libid3tag
#
brew install \
    adwaita-icon-theme \
    curl \
    chromaprint \
    flac \
    glib \
    gtk+3 \
    icu4c \
    json-c \
    libgcrypt \
    libogg \
    opus \
    librsvg \
    libvorbis \
    libxml2 \
    opusfile

sudo -v

# this only works on apple silicon
brew link --force icu4c
brew link --force libxml2
brew link --force curl

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
