#!/bin/bash
#
# Copyright 2023-2025 Brad Lanam Pleasant Hill CA
#
ver=3

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

if [[ $(uname -s) != Linux ]]; then
  echo "Not running on Linux"
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

cwd=$(pwd)

LOG=/tmp/bdj4-pre-build.log
> $LOG
gr=N
pkgprog=
pkgrm=
pkginst=
pkginstflags=
pkgconfirm=

if [[ -f /usr/bin/pacman ]]; then
  pkgprog=/usr/bin/pacman
  pkgrm=-R
  pkginst=-S
  pkginstflags=--needed
  pkgconfirm=--noconfirm
  pkgchk=
fi
if [[ -f /usr/bin/apt ]]; then
  pkgprog=/usr/bin/apt
  pkgrm=remove
  pkginst=install
  pkginstflags=
  pkgconfirm=-y
  pkgchk=
fi
if [[ -f /usr/bin/dnf ]]; then
  pkgprog=/usr/bin/dnf
  pkgrm=remove
  pkginst=install
  # fedora needs to have libcurl-minimal replaced with libcurl
  # these flags are dangerous
  pkginstflags="--best --allowerasing"
  pkgconfirm=-y
  pkgchk=info
fi
if [[ -f /usr/bin/zypper ]]; then
  pkgprog=/usr/bin/zypper
  pkgrm=remove
  pkginst=install
  pkginstflags=
  pkgconfirm=-y
  pkgchk=
fi
if [[ -f /sbin/apk ]]; then
  pkgprog=/sbin/apk
  pkgrm=remove
  pkginst=add
  pkginstflags=
  pkgconfirm=
  pkgchk=
fi

if [[ $pkgprog == "" ]]; then
  echo ""
  echo "This Linux distribution is not supported by this script."
  echo "You will need to manually install the required packages."
  echo ""
  exit 1
fi

sudo -v

if [[ -f /usr/bin/dnf ]]; then
  # redhat based linux (fedora/rhel/centos, dnf)
  echo "-- To install vlc, the 'rpmfusion' repository"
  echo "-- is required. Proceed with 'rpmfusion' repository installation?"
  gr=$(getresponse)
  if [[ $gr = Y ]]; then
    ostype=el
    if [[ -f /etc/fedora-release ]]; then
      ostype=fedora
    fi
    echo "== Install rpmfusion repositories" >> $LOG
    sudo $pkgprog $pkgconfirm $pkginst $pkginstflags \
        https://download1.rpmfusion.org/free/${ostype}/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm \
        https://download1.rpmfusion.org/nonfree/${ostype}/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm \
        >> $LOG 2>&1
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "  ** failed ($pkgprog install rpmfusion)"
      echo "  ** failed ($pkgprog install rpmfusion)" >> $LOG
    fi
  fi
fi

sudo -v

pkglist=""
if [[ -f /usr/bin/pacman ]]; then
  # arch based linux
  # tested
  # updated 2023-10-29
  # tested 2024-5-22
  # pre-installed: libogg, chromaprint, libopus, libopusfile, curl, ffmpeg
  # pre-installed: flac, libvorbis, json-c
  pkglist="rsync cmake make gcc gcc-objc pipewire curl gtk3 vlc pulseaudio
      libgcrypt libogg opus opusfile libvorbis flac ffmpeg4.4 check pkgconfig"
fi
if [[ -f /usr/bin/apt ]]; then
  # debian based linux
  # updated 2025-8-8
  # tested 2025-8-8
  pkglist="rsync cmake make gcc g++ gobjc check ffmpeg librsvg2-bin
      libgtk-3-dev libvlc-dev libvlccore-dev libpulse-dev
      libgcrypt-dev libogg-dev libopus-dev libopusfile-dev libvorbis-dev
      libflac-dev libavformat-dev libavutil-dev libxml2-dev libjson-c-dev
      libcurl4-openssl-dev libchromaprint-tools libgstreamer1.0-dev
      libglib2.0-dev libssl-dev"
  # on older linux, libpipewire-0.3 does not exist
  pkglist+=" libpipewire-0.3-dev"
fi
if [[ -f /usr/bin/dnf ]]; then
  # redhat/fedora
  # from the rpmfusion repository: vlc
  # tested
  # updated 2024-5-22
  # tested 2024-5-22
  # the installed libcurl is 'minimal' and should be replaced.
  # use ffmpeg-free, as the development libraries are only available from
  # the rpmfusion repository.
  # 38: pre-installed: libogg opus
  pkglist="rsync cmake make gcc gcc-c++ gcc-objc
      ffmpeg-free-devel libavformat-free-devel
      pipewire-devel libcurl-devel gtk3-devel vlc-devel pulseaudio-libs-devel
      openssl-devel libgcrypt-devel libogg-devel opus-devel opusfile-devel
      libvorbis-devel gstreamer-devel
      flac-devel check-devel json-c-devel
      chromaprint-tools"
fi
if [[ -f /usr/bin/zypper ]]; then
  # opensuse
  # updated 2024-4-22 (gcc13)
  # tested 2024-5-22
  sudo systemctl stop pkgkit
  sudo systemctl stop packagekit
  pkglist="rsync cmake make gcc13 gcc13-c++ gcc13-objc ffmpeg-4
      ffmpeg-4-libavformat-devel
      libpulse-devel gtk3-devel vlc-devel
      pipewire-devel libcurl-devel libgcrypt-devel libogg-devel libopus-devel
      opusfile-devel libvorbis-devel flac-devel check-devel
      libxml2-devel libjson-c-devel libvlc5 gstreamer-devel
      chromaprint-fpcalc"
fi
if [[ -f /sbin/apk ]]; then
  # alpine linux
  # updated 2025-4-18
  # tested never
  pkglist="rsync cmake make gcc g++ gcc-objc curl-dev
      ffmpeg4-dev pulseaudio-dev gtk+3.0-dev vlc-dev
      pipewire-dev curl-dev libgcrypt-dev libogg-dev opus-dev
      opusfile-dev libvorbis-dev flac-dev check-dev
      libxml2-dev libjson-c-dev gstreamer-dev
      chromaprint"
fi

sudo -v

rc=N
if [[ "$pkgprog" != "" && "$pkglist" != "" ]]; then
  echo "-- The following packages will be installed:"
  echo $pkglist | sed -e 's/.\{45\} /&\n    /g' -e 's/^/    /'
  echo "-- Proceed with package installation?"
  gr=$(getresponse)
fi

sudo -v

if [[ -f /usr/bin/pacman ]]; then
  # manjaro linux may have vlc-nightly installed.
  # want something more stable.
  echo "== Remove vlc-nightly" >> $LOG
  sudo $pkgprog $pkgrm $pkgconfirm vlc-nightly >> $LOG 2>&1
fi
if [[ -f /usr/bin/zypper ]]; then
  echo "== Remove gcc7" >> $LOG
  sudo $pkgprog $pkgrm $pkgconfirm gcc7 >> $LOG 2>&1
fi

sudo -v

rc=0
echo "== Install packages" >> $LOG
if [[ "$pkgprog" != "" && "$pkglist" != "" && $gr = Y ]]; then
  sudo $pkgprog $pkginst $pkgconfirm $pkginstflags $pkglist >> $LOG 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "  ** failed (install packages)"
    echo "  ** failed (install packages)" >> $LOG
  fi
fi

sudo -k

echo "** Pre-build log is located at: $LOG"
echo "Press enter to finish."
read answer
exit $rc
