#!/bin/sh
#
# Copyright 2009-2018 Brad Lanam Walnut Creek, CA USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
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

RUNTOPDIR=`pwd`
MKC_FILES=${MKC_FILES:-mkc_files}
CACHEFILE="${MKC_FILES}/mkconfig.cache"

unset GREP_OPTIONS
unset ENV

getlibdata () {
    var=$1
    gdname=$2
    lang=$3

    cmd="${var}=\${mkc_${lang}_lib_${gdname}}"
    eval $cmd
}

mkconfigversion

debug=F
OUTLIBFILE="${MKC_FILES}/mkconfig.reqlibs"
while test $# -gt 1; do
  case $1 in
    -X)
      shift
      debug=T
      ;;
    -c)
      shift
      CACHEFILE=$1
      shift
      ;;
    -o|-l)   # -l backwards compatibility
      shift
      OUTLIBFILE=$1
      shift
      ;;
  esac
done

CONFH=$1

ok=1
if [ ! -f "${CONFH}" ]; then
  echo "Unable to locate ${CONFH}"
  ok=0
fi
if [ ! -f "$RUNTOPDIR/$CACHEFILE" ]; then
  echo "Unable to locate $RUNTOPDIR/$CACHEFILE"
  ok=0
fi
if [ $ok -eq 0 ]; then
  exit 1
fi

reqlibs=""
. $RUNTOPDIR/$CACHEFILE

exec 7<&0 < ${CONFH}
dver=0
while read cline; do
  if [ $debug = T ]; then
    echo "cline:$cline:"
  fi
  case $cline in
    "#define _lib_"*1)
      lang=c
      ;;
    "enum bool _clib_"*" = true;")
      lang=d
      dver=2
      ;;
    "enum bool _lib_"*" = true;")
      lang=d
      dver=2
      ;;
    "enum : bool { _clib_"*" = true };")
      lang=d
      dver=1
      ;;
    "enum : bool { _lib_"*" = true };")
      lang=d
      dver=1
      ;;
    *)
      continue
      ;;
  esac

  # bash2 can't handle # in subst
  if [ $lang = "d" -a $dver -eq 1 ]; then
    dosubst cline ': ' '' '{ ' '' ' }' ''
  fi
  dosubst cline '#define ' '' ' 1' '' ' = true;' '' 'enum bool ' ''
  getlibdata var $cline $lang
  if [ $debug = T ]; then
    echo "cline:$cline:lang:$lang:var:$var:"
  fi
  if [ "$var" != "" ]; then
    echo $reqlibs | grep -- $var > /dev/null 2>&1
    rc=$?
    if [ $rc -ne 0 ]; then
      doappend reqlibs " $var"
      if [ $debug = T ]; then
        echo "append:$var"
      fi
    fi
  fi
done
exec <&7 7<&-
echo $reqlibs > $OUTLIBFILE
