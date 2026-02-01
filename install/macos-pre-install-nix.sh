#!/bin/bash
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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

skipnixinst=F
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

LOGF=/tmp/bdj4-install-nix.log
echo "-- Logging to $LOGF"
> $LOGF

sudo -v

pfx=/nix/var/nix/profiles/default

if [[ $skipnixinst == F ]]; then
  nix_installed=F
  if [[ -d ${pfx} ]]; then
    nix_installed=T
  else
    echo "-- NIX is not installed"
  fi

  if [[ $nix_installed == F ]]; then
    echo "-- Installing NIX"
    of=nix-installer
    curl --silent --output ${of} \
        -L https://install.determinate.systems/determinate-pkg/stable/Universal \
        >> $LOGF 2>&1
    chmod a+rx ${of}
    xattr -d com.apple.quarantine ${of} > /dev/null 2>&1
    sudo ./${of} install --yes >> $LOGF 2>&1
    rm -f ./${of}
  fi
fi

if [[ ! -d ${pfx} ]]; then
  echo "NIX failed to install"
  exit 1
fi

sudo -v

if [[ -d ${pfx} ]]; then
  nixbin=${pfx}/bin
fi
PATH=$PATH:${nixbin}/bin
NIXSH=${nixbin}/nix-shell

if [ -e "${pfx}/etc/profile.d/nix-daemon.sh" ]; then
  . "${pfx}/etc/profile.d/nix-daemon.sh"
fi

nix --version > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "NIX failed to install"
  exit 1
fi

TLOC=${TMPDIR:-/var/tmp}
TFN="$TLOC/bdj4-pre-inst.tmp"

sudo -v

pkgs="nixpkgs#adwaita-icon-theme \
  nixpkgs#curl \
  nixpkgs#chromaprint \
  nixpkgs#flac \
  nixpkgs#glib \
  nixpkgs#gtk3 \
  nixpkgs#icu \
  nixpkgs#json_c \
  nixpkgs#libgcrypt \
  nixpkgs#libogg \
  nixpkgs#libopus \
  nixpkgs#librsvg \
  nixpkgs#libvorbis \
  nixpkgs#libxml2 \
  nixpkgs#opusfile"

sudo -v

nix profile add "${pkgs}"

sudo -v

nix profile upgrade

sudo -v

echo "-- Cleaning nix"
nix-env --delete-generations old >> $LOGF 2>&1

sudo -k

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
  ${NIXSH} --run "bash ${latest}"
fi

echo "-- Log file: $LOGF"

exit 0
