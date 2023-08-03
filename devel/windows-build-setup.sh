#!/bin/bash

pacman -Sy \
    make \
    rsync \
    vim \
    tar \
    unzip \
    zip \
    diffutils openssh dos2unix \
    gettext gettext-devel \
    mingw-w64-x86_64-gcc \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-pkgconf \
    mingw-w64-x86_64-gcc-objc \
    mingw-w64-x86_64-gtk3 \
    mingw-w64-x86_64-icu \
    mingw-w64-x86_64-python-pip \
    mingw-w64-x86_64-python-wheel
pip3 install --user --upgrade mutagen
