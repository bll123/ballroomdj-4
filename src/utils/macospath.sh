#!/bin/bash
#
# Copyright 2025 Brad Lanam Pleasant Hill CA
#

npath=$(echo $PATH | sed 's,/opt/local/bin:,,g')
npath=$(echo $npath | sed 's,/opt/local/sbin:,,g')
npath=$(echo $npath | sed 's,/opt/homebrew/bin:,,g')
if [[ $1 == brew ]]; then
  npath=/opt/homebrew/bin:$npath
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
