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
${bdir}/macos-pre-install-pkgsrc.sh

sudo -v

sudo pkgin -y install cmake check pkgconf

sudo -k

echo "Press enter to finish."
read answer
exit $rc
