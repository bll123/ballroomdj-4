#!/bin/bash

systype=$(uname -s)
case $systype in
  Linux)
    ;;
  Darwin)
    exit 1
    ;;
  MINGW64*)
    exit 1
    ;;
esac

# update system
sudo apt update
sudo apt -y full-upgrade
sudo apt -y autoremove

# programming tools / compilers / libraries / other tools
sudo apt -y install \
    mercurial tortoisehg \
    ffmpeg librsvg2-bin libcurl4 \
    libogg-dev libopus-dev libopusfile-dev \
    libavcodec-dev libavformat-dev libavutil-dev \
    g++ gobjc patchelf clang clang-tools gcc-doc cmake \
    check libgtk-3-dev libvlc-dev libvlccore-dev libpulse-dev \
    libasound2-dev libcurl4-openssl-dev libxml2-dev \
    gtk-3-examples \
    python3-mutagen \
    dos2unix sshpass

# inkscape / fonts (for editing bdj4 images)
sudo apt -y install inkscape \
    fonts-roboto fonts-tlwg-kinnari fonts-texgyre
    fonts-tibetan-machine fonts-kacst \
    fonts-knda fonts-sil-paduak fonts-telu \
