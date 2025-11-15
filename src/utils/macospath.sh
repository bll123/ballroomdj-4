#!/bin/bash
#
# Copyright 2025 Brad Lanam Pleasant Hill CA
#

npath=$(echo $PATH | sed 's,/opt/local/bin:,,g')
npath=$(echo $npath | sed 's,/opt/local/sbin:,,g')
npath=$(echo $npath | sed 's,/opt/homebrew/bin:,,g')
npath=$(echo $npath | sed 's,/opt/homebrew/sbin:,,g')
npath=$(echo $npath | sed 's,/usr/local/bin:,,g')
npath=$(echo $npath | sed 's,/usr/local/sbin:,,g')
if [[ $1 == brew ]]; then
  if [[ -d /opt/homebrew ]]; then
    pfx=/opt/homebrew
  fi
  if [[ -d /usr/local/Homebrew ]]; then
    pfx=/usr/local
  fi
  npath=${pfx}/bin:$npath
  PATH=$npath
  unset npath
  if [[ -d ../data ]]; then
    touch ../data/macos.homebrew
  fi
  if [[ -d data ]]; then
    touch data/macos.homebrew
  fi
fi
if [[ $1 == macports ]]; then
  npath=/opt/local/bin:$npath
  PATH=$npath
  unset npath
  if [[ -d ../data ]]; then
    rm -f ../data/macos.homebrew
  fi
  if [[ -d data ]]; then
    rm -f data/macos.homebrew
  fi
fi
