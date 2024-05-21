#!/bin/bash
#
# Copyright 2023-2024 Brad Lanam Pleasant Hill CA
#
ver=2

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

LOG=/tmp/bdj4-pre-build.log
> $LOG

bdir=$(dirname $0)
${bdir}/macos-pre-install-macports.sh

sudo -v

sudo port uninstall -f gtk3
sudo port -N install gtk3-devel +quartz
sudo port -N install cmake check

sudo -k

echo "** Pre-build log is located at: $LOG"
echo "Press enter to finish."
read answer
exit $rc
