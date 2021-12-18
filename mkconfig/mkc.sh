#!/bin/sh
#
# Copyright 2009-2018 Brad Lanam Walnut Creek, CA USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#

# this is a workaround for ksh93 on solaris
unset CDPATH
if [ "$1" = "-d" ]; then
  cd $2
  shift
  shift
fi
mypath=`echo $0 | sed -e 's,/[^/]*$,,' -e 's,^\.,./.,'`
_MKCONFIG_DIR=`(cd $mypath;pwd)`
export _MKCONFIG_DIR
. ${_MKCONFIG_DIR}/bin/shellfuncs.sh
. ${_MKCONFIG_DIR}/bin/envfuncs.sh

doshelltest $0 $@

if [ "$MKC_PREFIX" != "" ]; then
  if [ "$MKC_CONFDIR" = "" ]; then
    MKC_CONFDIR=.
    export MKC_CONFDIR
  else
    test -d "${MKC_CONFDIR}" || mkdir -p "${MKC_CONFDIR}"
  fi
  if [ "$MKC_OUTPUT" = "" ]; then
    MKC_OUTPUT=config.h
    export MKC_OUTPUT
  fi
  if [ ! -f ${MKC_CONFDIR}/${MKC_PREFIX}.env ]; then
    ${_MKCONFIG_SHELL} ${MKC_DIR}/mkconfig.sh ${MKC_CONFDIR}/${MKC_PREFIX}-env.mkc
  fi
  if [ -f ${MKC_PREFIX}.env ]; then
    . ./${MKC_PREFIX}.env
    if [ ! -f ${MKC_OUTPUT} ]; then
      ${_MKCONFIG_SHELL} ${MKC_DIR}/mkconfig.sh ${MKC_CONFDIR}/${MKC_PREFIX}.mkc
    fi
  fi
fi

rc=0
args=$@
found=T
case $1 in
  -compile|-comp|-link)
    ${_MKCONFIG_SHELL} ${_MKCONFIG_DIR}/bin/mkcl.sh -d `pwd` "$@"
    rc=$?
    shift
    ;;
  -staticlib)
    shift
    ${_MKCONFIG_SHELL} ${_MKCONFIG_DIR}/bin/mkstaticlib.sh -d `pwd` "$@"
    rc=$?
    ;;
  -setopt)
    shift
    ${_MKCONFIG_SHELL} ${_MKCONFIG_DIR}/bin/mkcsetopt.sh -d `pwd` "$@"
    rc=$?
    ;;
  -reqlib)
    shift
    ${_MKCONFIG_SHELL} ${_MKCONFIG_DIR}/bin/mkreqlib.sh -d `pwd` "$@"
    rc=$?
    ;;
  -makeconfig)
    rc=0
    ;;
  *)
    found=F
    ;;
esac

if [ $found = F ]; then
  echo "Usage: $0 {-compile|-link|-setopt|-reqlib|-staticlib|-sharedlib} <args>"
  exit 1
fi
exit $rc
