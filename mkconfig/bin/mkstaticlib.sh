#!/bin/sh
#
# Copyright 2010-2018 Brad Lanam Walnut Creek, CA USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#

unset CDPATH
# this is a workaround for ksh93 on solaris
if [ "$1" = "-d" ]; then
  cd $2
  shift
  shift
fi
. ${_MKCONFIG_DIR}/bin/shellfuncs.sh
doshelltest $0 $@

libnm=""
objects=""
grc=0
doecho=F
MKC_FILES=${MKC_FILES:-mkc_files}
logfile=${MKC_FILES}/mkc_compile.log
while test $# -gt 0; do
  case $1 in
    -e)
      shift
      doecho=T
      ;;
    --)
      shift
      ;;
    -log)
      shift
      logfile=$1
      shift
      ;;
    -o)
      shift
      tf=$1
      shift
      if [ "$libnm" = "" ]; then
        libnm=$tf
        continue
      fi
      ;;
    *${OBJ_EXT})
      tf=$1
      shift
      if [ ! -f "$tf" ]; then
        puts "## unable to locate $tf"
        grc=1
      else
        doappend objects " $tf"
      fi
      ;;
    *)
      tf=$1
      shift
      if [ "$libnm" = "" ]; then
        libnm=$tf
        continue
      fi
      ;;
  esac
done

if [ "$logfile" != "" ]; then
  exec 9>>$logfile
  puts "# `date`" >&9
fi

locatecmd ranlibcmd ranlib
locatecmd arcmd ar
locatecmd lordercmd lorder
locatecmd tsortcmd tsort

if [ "$arcmd" = "" ]; then
  puts "## Unable to locate 'ar' command"
  grc=1
fi

if [ $grc -eq 0 ]; then
  dosubst libnm '${SHLIB_EXT}$' ''
  libfnm=${libnm}
  # for really old systems...
  if [ "$ranlibcmd" = "" -a "$lordercmd" != "" -a "$tsortcmd" != "" ]; then
    objects=`$lordercmd ${objects} | $tsortcmd`
  fi
  test -f $libfnm && rm -f $libfnm
  cmd="$arcmd cq $libfnm ${objects}"
  putsnonl "CREATE ${libfnm} ..."
  if [ "$logfile" != "" ]; then
    puts "CREATE ${libfnm}" >&9
  fi
  if [ $doecho = "T" ]; then
    puts ""
    puts $cmd
  fi
  if [ "$logfile" != "" ]; then
    out=`eval $cmd 2>&1`
    rc=$?
    puts "$out" >&9
    if [ $doecho = T ]; then
      puts "$out"
    fi
  else
    eval $cmd
    rc=$?
  fi
  if [ $rc -ne 0 ]; then
    grc=$rc
  fi
  if [ "$ranlibcmd" != "" ]; then
    cmd="$ranlibcmd $libfnm"
    if [ $doecho = "T" ]; then
      puts $cmd
    fi
    if [ "$logfile" != "" ]; then
      eval $cmd >&9
      rc=$?
    else
      eval $cmd
      rc=$?
    fi
  fi
  if [ $rc -ne 0 ]; then
    puts " fail"
    grc=$rc
  else
    puts " ok"
  fi
fi

if [ "$logfile" != "" ]; then
  exec 9>&-
fi
exit $grc
