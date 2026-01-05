#!/bin/bash
#
# Copyright 2023-2026 Brad Lanam Pleasant Hill CA
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

if [[ $(uname -m) == x86_64 ]]; then
  echo "Homebrew cannot be used on Intel MacOS."
  echo "  On Intel MacOS, Homebrew does not make the necessary libraries available."
  exit 1
fi

cwd=$(pwd)

bdir=$(dirname $0)
${bdir}/macos-pre-install-homebrew.sh

PATH=$PATH:/opt/homebrew/bin

brew install cmake check pkgconf

echo "Press enter to finish."
read answer
exit $rc
