#!/bin/bash
#
# Copyright 2023-2025 Brad Lanam Pleasant Hill CA
#

# exported variables
# systype   - uname -s
# arch      - uname -m
# archtag   - macos: intel/applesilicon
# procs     - number of available processors
# tag       - platform tag name
# platform  - type of platform
# CC        - compiler
# CXX       - c++ compiler
# cmg       - cmake -G type
# cmpbld    - cmake parallel build
# esuffix   - executable suffix

procs=$(getconf _NPROCESSORS_ONLN)
echo "= procs: $procs"

export PKG_CONFIG_PATH=$INSTLOC/lib/pkgconfig

systype=$(uname -s)
arch=$(uname -m)
esuffix=
case $systype in
  Linux)
    tag=linux
    platform=unix
    archtag=
    CC=gcc
    CXX=g++
    if [[ -f /usr/bin/gcc-13 && ! -f /usr/bin/gcc-15 ]]; then
      CC=gcc-13
      CXX=g++-13
    fi
    cmg="Unix Makefiles"
    cmpbld="--parallel"
    ;;
  Darwin)
    tag=macos
    platform=unix
    CC=clang
    CXX=clang++
    case $arch in
      x86_64)
        archtag=-intel
        ;;
      arm64)
        archtag=-applesilicon
        ;;
    esac
    cmg="Unix Makefiles"
    cmpbld="--parallel"
    ;;
  MINGW64*)
    tag=win64
    platform=windows
    archtag=
    esuffix=.exe
    CC=gcc
    CXX=g++
    cmg="MSYS Makefiles"
    cmpbld=""
    CFLAGS="-m64 -O2"
    LDFLAGS=-m64
    PKG_CFLAGS=-m64
    export CFLAGS PKG_CFLAGS LDFLAGS
    export LIBS="-static-libgcc"
    ;;
  MINGW32*)
    echo "Platform not supported"
    exit 1
    ;;
esac

export systype procs tag platform arch archtag cmg cmpbld esuffix
export CC CXX
