#!/bin/bash
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
ver=25

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

MPCHKTXT=mpchk.txt
MPVERS=macports_version

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

skipmpinst=F
oldmacos=F

case $vers in
  10.*)
    # in version 10, the macos release name changes
    # were based on the second value.
    mp_os_vers=$(echo $vers | sed 's,^10\.\([0-9]*\).*,10.\1,')
    ;;
  *)
    mp_os_vers=$(echo $vers | sed 's,\..*,,')
    ;;
esac

case $mp_os_vers in
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
    # grab the latest version number
    rsync -q rsync://rsync.macports.org/release/base/config/${MPVERS} .
    if [[ -f ${MPVERS} ]]; then
      mp_vers=$(cat ${MPVERS})
      rm -f ${MPVERS}
    else
      echo "Unable to connect to macports.org"
      exit 1
    fi

    # the chk.txt file is a messy way to locate the macports name,
    # but it works.
    baseurl=https://github.com/macports/macports-base/releases/download
    url=${baseurl}/v${mp_vers}/MacPorts-${mp_vers}.chk.txt
    curl -L --silent --output ${MPCHKTXT} ${url}
    if [[ ! -f ${MPCHKTXT} ]]; then
      echo "Unable to connect to github.com/macports"
      exit 1
    fi

    while read line ; do
      case ${line} in
        *MacPorts-${mp_vers}-${mp_os_vers}-*.pkg*)
          mp_os_nm=$(echo $line |
              sed -e "s,.*MacPorts-${mp_vers}-${mp_os_vers}-,," \
              -e 's,\.pkg.*,,')
          break
          ;;
      esac
    done < ${MPCHKTXT}
    test -f ${MPCHKTXT} && rm -f ${MPCHKTXT}

    pkgnm=MacPorts-${mp_vers}-${mp_os_vers}-${mp_os_nm}.pkg
    echo "-- Downloading MacPorts"
    curl --silent -JOL ${baseurl}/v${mp_vers}/${pkgnm}
    echo "-- Installing MacPorts using sudo"
    sudo installer -pkg ${pkgnm} -target /Applications
    rm -f ${pkgnm}
  fi
fi

PATH=$PATH:/opt/local/bin

sudo -v

echo "-- Running MacPorts 'port selfupdate' with sudo"
sudo port -N selfupdate

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
sudo port -N upgrade outdated

sudo -v

echo "-- Installing packages needed by BDJ4"
# using our own libid3tag
# 2024-6-7 mesa is now needed to be able to build gtk3
#   a bug was opened on macports.
sudo port -N install \
    adwaita-icon-theme \
    chromaprint \
    curl \
    curl-ca-bundle \
    ffmpeg +nonfree -x11 -rav1e \
    flac \
    glib2 +quartz \
    gtk3 +quartz \
    icu \
    json-c \
    libgcrypt \
    libogg \
    libopus \
    librsvg \
    libvorbis \
    libxml2 \
    mesa \
    mpstats \
    opusfile

sudo -v

if [[ -z "$(port -q list inactive)" ]]; then
  sudo port -N reclaim --disable-reminders --keep-build-deps
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
