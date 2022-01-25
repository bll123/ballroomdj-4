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

addlib () {
  lfn=$1

  found=F
  nlibnames=
  for tf in $libnames; do
    if [ $lfn = $tf ]; then
      found=T   # a match will be moved to the end of the list
    else
      doappend nlibnames " $tf"
    fi
  done
  doappend nlibnames " ${lfn}"
  libnames=${nlibnames}
}

addlibpath () {
  lp=$1

  found=F
  for tp in $libpathnames; do
    if [ $lp = $tp ]; then
      found=T   # a match will be kept.
      break
    fi
  done
  if [ $found = F ]; then
    doappend libpathnames " ${lp}"
  fi
}

addrunpath () {
  lp=$1

  found=F
  for tp in $runpathnames; do
    if [ $lp = $tp ]; then
      found=T   # a match will be kept.
      break
    fi
  done
  if [ $found = F ]; then
    doappend runpathnames " ${lp}"
  fi
}

compile=F
link=F
shared=F
mkexec=F
mklib=F

doecho=F
comp=${CC}
reqlibfiles=
MKC_FILES=${MKC_FILES:-mkc_files}
logfile=${MKC_FILES}/mkc_compile.log
c=T
d=F
while test $# -gt 0; do
  case $1 in
    -compile|-comp)
      shift
      compile=T
      shared=F
      ;;
    -link)
      shift
      link=T
      ;;
    -log)
      shift
      logfile=$1
      shift
      ;;
    -shared)
      shift
      shared=T
      ;;
    -exec)
      shift
      mkexec=T
      ;;
    -lib)
      shift
      mkexec=F
      mklib=T
      ;;
    -c)
      shift
      comp=$1
      shift
      case ${comp} in
        *gdc*|*ldc*|*dmd*)
          d=T
          c=F
          ;;
      esac
      ;;
    -d)
      shift
      ndir=$1
      cd $ndir
      rc=$?
      if [ $rc -ne 0 ]; then
        puts "## Unable to cd to $ndir"
        grc=$rc
        exit $grc
      fi
      shift
      ;;
    -e)
      doecho=T
      shift
      ;;
    -o)
      shift
      outfile=$1
      shift
      ;;
    -r)
      shift
      doappend reqlibfiles " $1"
      shift
      ;;
    [A-Za-z0-9_][A-Za-z0-9_]*=[\'\"]*[\'\"])
      eval $1
      shift
      ;;
    -D*|-U*)
      doappend CFLAGS_USER " $1"
      shift
      ;;
    -Wc*)
      doappend CFLAGS_APPLICATION " $1"
      ;;
    --)
      shift
      break
      ;;
    *)
      break
      ;;
  esac
done
if [ "$logfile" != "" ]; then
  exec 9>>$logfile
  puts "# `date`" >&9
fi

# DC_LINK should be in environment already.
OUTFLAG="-o "
DC_LINK=
case ${comp} in
  *dmd*|*ldc*)   # catches ldmd, ldmd2 also
    OUTFLAG=${DC_OF:-"-of"}
    DC_LINK=-L
    ;;
  *gdc*)
    OUTFLAG=${DC_OF:-"-o "}
    ;;
  *gcc*|*cc*)
    DC_LINK=
    ;;
esac

flags=
files=
objects=
libnames=
libpathnames=
runpathnames=
islib=0
ispath=0
isrpath=0
olibs=
havesource=F

if [ "$reqlibfiles" != "" ]; then
  for rf in $reqlibfiles; do
    doappend olibs "`cat $rf` "
  done
fi

grc=0
for f in $@ $olibs; do
  case $f in
    -D*)
      doappend CFLAGS_USER $1
      shift
      ;;
    -L)
      ispath=1
      ;;
    -L*)
      tf=$f
      dosubst tf '-L' ''
      if [ ! -d "$tf" ]; then
        puts "## unable to locate dir $tf"
      else
        addlibpath $tf
      fi
      ;;
    -R|-rpath)
      isrpath=1
      ;;
    -R*|-rpath*)
      tf=$f
      dosubst tf '-rpath' ''
      if [ ! -d "$tf" ]; then
        puts "## unable to locate dir $tf"
      else
        addrunpath $tf
      fi
      ;;
    -l)
      islib=1
      ;;
    -l*)
      addlib $f
      ;;
    -Wl*)
      addlib $f
      ;;
    lib*)
      addlib $f
      ;;
    *${OBJ_EXT})
      if [ ! -f "$f" ]; then
        puts "## unable to locate $f"
        grc=1
      else
        doappend objects " $f"
      fi
      ;;
    *.c|*.d|*.m|*.cpp|*.res)
      if [ ! -f "$f" ]; then
        puts "## unable to locate $f"
        grc=1
      else
        doappend files " $f"
        havesource=T
      fi
      ;;
    "-"*)
      doappend flags " $f"
      if [ $f = "-c" ]; then
        compile=T
      fi
      ;;
    *)
      if [ $islib -eq 1 ]; then
        addlib "-l$f"
      elif [ $isrpath -eq 1 ]; then
        if [ ! -d "$f" ]; then
          puts "## unable to locate dir $f"
          grc=1
        else
          addrunpath $f
        fi
      elif [ $ispath -eq 1 ]; then
        if [ ! -d "$f" ]; then
          puts "## unable to locate dir $f"
          grc=1
        else
          addlibpath $f
        fi
      fi
      islib=0
      ispath=0
      isrpath=0
      ;;
  esac
done

libs=
for lfn in $libnames; do
  doappend libs " ${DC_LINK}${lfn}"
done

LDFLAGS_LIBPATH=-L
libpath=
for lp in $libpathnames; do
  doappend libpath " ${DC_LINK}${LDFLAGS_LIBPATH}${lp}"
done

runpath=
for lp in $runpathnames; do
  doappend runpath " ${LDFLAGS_RUNPATH}${lp}"
done

outflags=""
if [ "$outfile" = "" ]; then
  flags="${flags} -c"
else
  outflags="${OUTFLAG}$outfile"
  case ${outfile} in
    *.o|*.obj)
      flags="${flags} -c"
      ;;
  esac
fi

allcflags=
if [ $havesource = T ]; then
  if [ $c = T ];then
    if [ "$CFLAGS_ALL" != "" ]; then
      doappend allcflags " ${CFLAGS_ALL}"
    else
      doappend allcflags " ${CFLAGS_OPTIMIZE}"     # optimization flags
      doappend allcflags " ${CFLAGS_DEBUG}"        # debug flags
      doappend allcflags " ${CFLAGS_INCLUDE}"      # any include files
      doappend allcflags " ${CFLAGS_USER}"         # specified by the user
      if [ $shared = T ];then
        doappend allcflags " ${CFLAGS_SHARED}"
        doappend allcflags " ${CFLAGS_SHARED_USER}"
      fi
      doappend allcflags " ${CFLAGS_APPLICATION}"  # added by the config process
      doappend allcflags " ${CFLAGS_COMPILER}"     # compiler flags
      doappend allcflags " ${CFLAGS_SYSTEM}"       # needed for this system
    fi
  fi
  if [ $d = T ];then
    doappend allcflags " ${DFLAGS}"
  fi
fi

allldflags=
ldflags_libpath=
ldflags_runpath=
ldflags_shared_libs=
ldflags_exec_link=

if [ $link = T ]; then
  if [ "$LDFLAGS_ALL" != "" ]; then
    doappend allldflags " ${LDFLAGS_ALL}"
  else
    doappend allldflags " ${LDFLAGS_OPTIMIZE}"     # optimization flags
    doappend allldflags " ${LDFLAGS_DEBUG}"        # debug flags
    doappend allldflags " ${LDFLAGS_USER}"         # specified by the user
    doappend allldflags " ${LDFLAGS_APPLICATION}"  # added by the config process
    doappend allldflags " ${LDFLAGS_COMPILER}"     # link flags
    doappend allldflags " ${LDFLAGS_SYSTEM}"       # needed for this system
  fi
fi
if [ $link = T -a $shared = T ]; then
  doappend allldflags " ${LDFLAGS_SHARED_LIBLINK}"
  doappend allldflags " ${LDFLAGS_SHARED_USER}"
fi
if [ \( $shared = T \) -o \( $mkexec = T \) ]; then
  ldflags_libpath=""
  if [ "${libpath}" != "" -a "${LDFLAGS_LIBPATH}" != "" ]; then
    ldflags_libpath="${libpath}"
  fi
  ldflags_runpath=""
  if [ "${runpath}" != "" -a "${LDFLAGS_RUNPATH}" != "" ]; then
    ldflags_runpath="${runpath}"
  fi

  ldflags_shared_libs=""
  if [ "${libs}" != "" -a "${libpath}" != "" ]; then
    ldflags_shared_libs="${libpath}"
  fi

  ldflags_exec_link=""
  if [ $mklib = F -a $mkexec = T ]; then
    if [ "${LDFLAGS_EXEC_LINK}" != "" ]; then
      ldflags_exec_link="${LDFLAGS_EXEC_LINK}"
    fi
  fi
fi
if [ $link = T ]; then
  if [ "$LDFLAGS_LIBS_ALL" != "" ]; then
    doappend alllibs " ${LDFLAGS_LIBS_ALL}"
  else
    doappend alllibs " ${LDFLAGS_LIBS_USER}"
    doappend alllibs " ${LDFLAGS_LIBS_APPLICATION}"
    doappend alllibs " ${LDFLAGS_LIBS_SYSTEM}"
  fi
fi

# clear the standard env vars so they don't get picked up.
CFLAGS=
CPPFLAGS=
LDFLAGS=
LIBS=
cmd="${comp} ${allcflags} ${flags} ${allldflags} ${ldflags_exec_link} \
    $outflags $objects \
    ${files} ${ldflags_libpath} ${ldflags_runpath} \
    ${ldflags_shared_libs} ${alllibs} ${libs}"
disp=""
if [ $compile = T ]; then
  disp="${disp}COMPILE ${files} ... "
  if [ "$logfile" != "" ]; then
    puts "COMPILE ${files}" >&9
  fi
fi
if [ $link = T ]; then
  disp="${disp}LINK ${outfile} ... "
  if [ "$logfile" != "" ]; then
    puts "LINK ${outfile}" >&9
  fi
fi
if [ $doecho = T ]; then
  puts ""
  puts "    $cmd"
fi
if [ "$logfile" != "" ]; then
  puts "    $cmd" >&9
fi
out=""
if [ "$logfile" != "" ]; then
  out=`eval $cmd 2>&1`
  rc=$?
  puts "$out" >&9
  if [ $doecho = F -a $rc -ne 0 ]; then
    puts ""
    puts "    $cmd"
    puts "$out"
  fi
  if [ $doecho = T ]; then
    puts "$out"
  fi
else
  eval $cmd
  rc=$?
fi
if [ $rc -eq 0 ]; then
  puts "$out" | grep -i warning: >/dev/null 2>&1
  rc=$?
  if [ $rc -eq 0 ]; then
    if [ $doecho = F ]; then
      puts ""
      puts "    $cmd"
      puts "$out"
    fi
    disp="${disp} warnings"
  else
    disp="${disp} ok"
  fi
else
  disp="${disp} fail"
  grc=$rc
fi
puts $disp

if [ "$logfile" != "" ]; then
  exec 9>&-
fi
exit $grc
