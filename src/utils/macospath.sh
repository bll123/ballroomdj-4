#!/bin/bash
#
# Copyright 2025 Brad Lanam Pleasant Hill CA
#

#
# this script must re-build any shipped packages that are
# affected by the package management system
# macos only builds libid3tag and libmp4tag,
# libmp4tag does not require any external libraries at this time
# and does not need to be rebuilt.

npath=$(echo $PATH | sed 's,/opt/local/bin:,,g')
npath=$(echo $npath | sed 's,/opt/local/sbin:,,g')
npath=$(echo $npath | sed 's,/opt/homebrew/bin:,,g')
npath=$(echo $npath | sed 's,/opt/homebrew/sbin:,,g')
npath=$(echo $npath | sed 's,/usr/local/bin:,,g')
npath=$(echo $npath | sed 's,/usr/local/sbin:,,g')
npath=$(echo $npath | sed 's,/opt/pkg/bin:,,g')
npath=$(echo $npath | sed 's,/opt/pkg/sbin:,,g')

if [[ -d data ]]; then
  dpath=devel
  ppath=pkg
fi
if [[ -d ../data ]]; then
  dpath=../devel
  ppath=../pkg
fi
homebrew=${dpath}/macos.homebrew
pkgsrc=${dpath}/macos.pkgsrc

if [[ $1 == macports ]]; then
  npath=/opt/local/bin:$npath
  PATH=$npath
  unset npath
  if [[ -f ${homebrew} || -f ${pkgsrc} ]]; then
    # only libid3tag needs to be re-built
    ${ppath}/build-all.sh --pkg libid3tag
  fi
  rm -f ${homebrew} ${pkgsrc}
fi

if [[ $1 == pkgsrc ]]; then
  npath=/opt/pkg/bin:$npath
  PATH=$npath
  unset npath
  if [[ ! -f ${pkgsrc} ]]; then
    # only libid3tag needs to be re-built
    ${ppath}/build-all.sh --pkg libid3tag
  fi
  rm -f ${homebrew} ${pkgsrc}
  touch ${pkgsrc}
fi

if [[ $1 == brew || $1 == homebrew ]]; then
  if [[ -d /opt/homebrew ]]; then
    pfx=/opt/homebrew
  fi
  if [[ -d /usr/local/Homebrew ]]; then
    # homebrew on intel will never work
    pfx=/usr/local
  fi
  npath=${pfx}/bin:$npath
  PATH=$npath
  unset npath
  if [[ ! -f ${homebrew} ]]; then
    # only libid3tag needs to be re-built
    ${ppath}/build-all.sh --pkg libid3tag
  fi
  rm -f ${homebrew} ${pkgsrc}
  touch ${homebrew}
fi
