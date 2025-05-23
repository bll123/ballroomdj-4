#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
ver=12

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

LOG=/tmp/bdj4-pre-install.log
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

pipp=/usr/bin/pip
if [[ -f /usr/bin/pip3 ]]; then
  pipp=/usr/bin/pip3
fi
# remove any old mutagen installed for the user
${pipp} uninstall -y mutagen > /dev/null 2>&1
${pipp} uninstall -y --break-system-packages mutagen > /dev/null 2>&1

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
  # updated 2023-9-20
  # tested 2023-10-30
  # pre-installed: libogg, chromaprint, libopus, libopusfile, curl, ffmpeg
  # pre-installed: flac, libvorbis, json-c
  :
fi
if [[ -f /usr/bin/apt ]]; then
  # debian based linux
  # tested 2023-10-29
  # updated 2023-10-25
  pkglist="ffmpeg libcurl4 libogg0 libopus0
      libopusfile0 libchromaprint-tools libvorbis0a libvorbisfile3
      flac libjson-c5"
fi
if [[ -f /usr/bin/dnf ]]; then
  # redhat/fedora
  # from the rpmfusion repository: vlc
  # updated 2023-9-20
  # tested 2023-10-30
  # the installed libcurl is 'minimal' and should be replaced.
  # use ffmpeg-free, as the development libraries are only available from
  # the rpmfusion repository.
  # 38: pre-installed: libogg opus
  pkglist="ffmpeg-free libcurl opusfile libvorbis
      flac-libs chromaprint-tools json-c"
fi
if [[ -f /usr/bin/zypper ]]; then
  # opensuse
  # updated 2023-10-30
  # tested 2023-10-30
  sudo systemctl stop pkgkit
  sudo systemctl stop packagekit
  pkglist="ffmpeg-4 libcurl4 libogg0 libopus0
      libopusfile0 libvorbis0 flac chromaprint-fpcalc libjson-c3"
fi
if [[ -f /sbin/apk ]]; then
  # alpine linux
  # updated 2025-4-18
  # tested never
  pkglist="ffmpeg4 libcurl4 libogg0 opus opusfile
      libvorbis flacc chromaprint libjson-c"
fi

sudo -v

pkglist="$pkglist vlc"
if [[ -f /usr/bin/apt || -f /usr/bin/zypper ]]; then
  pkglist="$pkglist libvlc5"
fi

rc=N
if [[ $pkgprog != "" && $pkglist != "" ]]; then
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

mutpkg=python3-mutagen
if [[ -f /usr/bin/pacman ]]; then
  mutpkg=python-mutagen
fi
sudo $pkgprog $pkgrm $pkgconfirm ${mutpkg} >> $LOG 2>&1

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

sudo -v

if [[ -f /usr/sbin/usermod ]]; then
  grep '^audio' /etc/group > /dev/null 2>&1
  rc=$?
  if [[ $rc -eq 0 ]]; then
    echo "-- Running sudo to add $USER to the audio group."
    # Not sure if this is necessary any more.  It was at one time.
    echo "-- add $USER to audio group" >> $LOG
    sudo /usr/sbin/usermod -a -G audio $USER >> $LOG 2>&1
  fi
fi

sudo -k

pconf=/etc/pulse/daemon.conf
upconf=$HOME/.config/pulse/daemon.conf
if [[ ! -f $pconf ]]; then
  if [[ -f $upconf ]]; then
    rm -f $upconf
  fi
fi
if [[ -f $pconf ]]; then
  grep -E '^flat-volumes *= *no$' $pconf > /dev/null 2>&1
  grc=$?
  urc=1
  if [[ -f $upconf ]]; then
    grep -E '^flat-volumes *= *no$' $upconf > /dev/null 2>&1
    urc=$?
  fi
  if [[ $grc -ne 0 && $urc -ne 0 ]]; then
    echo "-- reconfigure flat-volumes" >> $LOG
    echo "   grc: $grc urc: $urc" >> $LOG
    echo "Do you want to reconfigure pulseaudio to use flat-volumes (y/n)?"
    echo -n ": "
    read answer
    if [[ "$answer" = "y" || "$answer" = "Y" ]]; then
      if [[ -f $upconf ]]; then
        # $upconf exists
        grep -E '^flat-volumes' $upconf > /dev/null 2>&1
        rc=$?
        if [[ $rc -eq 0 ]]; then
          # there's already a flat-volumes configuration in $upconf, modify it
          sed -i '/flat-volumes/ s,=.*,= no,' $upconf >> $LOG 2>&1
        else
          echo "-- updating flat-volumes in $upconf" >> $LOG
          grep -E 'flat-volumes' $upconf > /dev/null 2>&1
          rc=$?
          if [[ $rc -eq 0 ]]; then
            # there exists some flat-volumes text in $upconf,
            # place the config change after that text.
            sed -i '/flat-volumes/a flat-volumes = no' $upconf  >> $LOG 2>&1
          else
            # no flat-volumes text in $upconf, just add it to the end.
            sed -i '$ a flat-volumes = no' $upconf  >> $LOG 2>&1
          fi
        fi
      else
        # $upconf does not exist at all
        echo 'flat-volumes = no' > $upconf
      fi
      killall pulseaudio  >> $LOG 2>&1
    fi
  fi
fi

echo "** Installation log is located at: $LOG"
echo "Press enter to finish."
read answer
exit $rc
