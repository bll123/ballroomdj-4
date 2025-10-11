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

if [[ $(uname -s) != Darwin ]]; then
  echo "Not running on MacOS"
  exit 1
fi

cwd=$(pwd)

bdir=$(dirname $0)
${bdir}/macos-pre-install-macports.sh

sudo -v

sudo port uninstall -f gtk3
sudo port -N install gtk3-devel +quartz
sudo port -N install cmake check

sudo -k

if [[ -z "$(port -q list inactive)" ]]; then
  sudo port -N reclaim --disable-reminders --keep-build-deps
fi

echo "Press enter to finish."
read answer
exit $rc
