#!/bin/bash
#
# Copyright 2023-2025 Brad Lanam Pleasant Hill CA
#
ver=1

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

case $(uname -s) in
  MINGW64*)
    case ${MSYSTEM} in
      UCRT64)
        ;;
      *)
        echo "Not a UCRT64 environment"
        exit 1
        ;;
    esac
    ;;
  *)
    echo "Not running on 64 bit windows"
    exit 1
    ;;
esac

cwd=$(pwd)

pacman -Syu --noconfirm
pacman -S --noconfirm --needed \
    make rsync vim tar unzip zip diffutils dos2unix \
    gettext gettext-devel \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-gcc-objc \
    mingw-w64-ucrt-x86_64-pkgconf \
    mingw-w64-ucrt-x86_64-gtk3 \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-icu \
    mingw-w64-ucrt-x86_64-json-c \
    mingw-w64-ucrt-x86_64-libgcrypt \
    mingw-w64-ucrt-x86_64-c-ares

exit 0
