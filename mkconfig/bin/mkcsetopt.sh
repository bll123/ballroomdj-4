#!/bin/sh
#
# Copyright 2010-2018 Brad Lanam Walnut Creek, CA USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#
# This script is used to change an option in an options file.
# Usage:
#   mkcsetopt.sh [-o options-file] option-name value
#

set -f

unset CDPATH
# this is a workaround for ksh93 on solaris
if [ "$1" = "-d" ]; then
  cd $2
  shift
  shift
fi
. ${_MKCONFIG_DIR}/bin/shellfuncs.sh
doshelltest $0 $@

OPTFILE=options.dat
while test $# -gt 1; do
  case $1 in
    -o)
      shift
      OPTFILE=$1
      shift
      ;;
    *)
      break
      ;;
  esac
done

opt=$1
val=$2

OPTNEW=options.new
exec 8>>${OPTNEW}
exec 7<&0 < ${OPTFILE}
while read oline; do
  case $oline in
    ${opt}=*)
      oline=`echo $oline | sed "s/=.*/=${val}/"`
      ;;
  esac
  echo $oline >&8
done
exec <&7 7<&-

mv ${OPTNEW} ${OPTFILE}
test -f ${OPTNEW} && rm -f ${OPTNEW}

exit $rc
