#!/bin/sh
#
# Copyright 1994-2018 Brad Lanam, Walnut Creek, CA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#

#
# speed at the cost of maintainability...
# File Descriptors:
#    9 - $TSTRUNLOG
#    8 - $MAINLOG
#    7 - $TMPORDER
#    5 - stdout (as 1 is directed to the log)
#

set -f # global

DOPERL=T
TESTORDER=test_order
SUBDIR=F

# this is a workaround for ksh93 on solaris
if [ "$1" = -d ]; then
  cd $2
  shift
  shift
fi
unset CDPATH

if [ "$1" = -s ]; then
  SUBDIR=T
  shift
  CC="$1"
  DC="$2"
  shelllist="$3"
  _pthlist="$4"
  TMPOUT="$5"
  shift; shift; shift; shift; shift
fi

if [ $# -lt 1 ]; then
  echo "Usage: $0 <test-dir> [<test-to-run> ...]"
  exit 1
fi

unset GREP_OPTIONS
unset ENV
unalias sed > /dev/null 2>&1
unalias grep > /dev/null 2>&1
unalias ls > /dev/null 2>&1
unalias rm > /dev/null 2>&1
LC_ALL=C
export LC_ALL

# this is used for regression testing.
getlistofshells () {

  getpaths
  echo "## PATH: $PATH" >&8
  echo "## paths: $_pthlist" >&8

  tshelllist=""
  inodelist=""
  for d in $_pthlist; do
    for s in $tryshell ; do
      rs=$d/$s
      if [ -x $rs ]; then
        while [ -h $rs -a $s != osh ]; do
          ors=$rs
          rs="`ls -l $rs | sed 's/.* //'`"
          case $rs in
            /*)
              ;;
            *)
              rs="$d/$rs"
              ;;
          esac

          # /etc/alternatives/xxx has some weird names w/dots.
          # all ksh* are ksh
          # anything not ksh/bash/zsh/osh is sh
          # if the name is sh->bash or sh->ksh; don't follow symlink.
          ts=`echo $rs | sed -e 's,.*[/\.],,' \
              -e 's/ksh88/ksh/' -e 's/ksh93/ksh/' `
          tts=`echo $s | sed -e 's/ksh88/ksh/' -e 's/ksh93/ksh/' `
          if [ $ts != ksh -a $ts != bash -a $ts != zsh ]; then
            ts=sh
          fi
          if [ $ts != ksh -a $tts != bash -a $tts != zsh ]; then
            tts=sh
          fi
          if [ $ts != $tts ]; then
            rs=$ors
            break
          fi

          rs=`echo $rs | sed 's,/[^/]*/\.\./,/,'`
          rs=`echo $rs | sed 's,/[^/]*/\.\./,/,'`
          rs=`echo $rs | sed 's,/[^/]*/\.\./,/,'`
          rs=`echo $rs | sed 's,/[^/]*/\.\./,/,'`
        done

        inode=`ls -i ${rs} | sed -e 's/^ *//' -e 's/ .*//'`
        inode=${inode}${s}   # append shell type also
        found=F
        for i in $inodelist; do
          if [ "$inode" = "${i}" ]; then
            found=T
            break
          fi
        done
        if [ $found = T ]; then
          continue
        fi

        if [ -x $rs ]; then
          cmd="$rs -c \". $_MKCONFIG_DIR/bin/shellfuncs.sh;getshelltype $rs echo\""
          set `eval $cmd`
          dispshell=$1
          echo "  found: $rs ($dispshell)" >&8
          case $dispshell in
            *)
              tshelllist="${tshelllist}
$rs"
              if [ "${inode}" != ${s} ]; then
                inodelist="${inodelist}
${inode}"
              fi
              ;;
          esac
        fi # if executable
      fi  # if there is a file
    done
  done

  tshelllist=`echo "$tshelllist" | sort -u`
  systype=`uname -s`
  shelllist=""
  for s in $tshelllist; do
    putsnonl "  check $s" >&8
    putsnonl "   $s"
    cmd="$s -c \". $_MKCONFIG_DIR/bin/shellfuncs.sh;chkshell echo $s\""
    eval $cmd >&8 2>&1
    rc=$?
    cmd="$s -c \". $_MKCONFIG_DIR/bin/shellfuncs.sh;getshelltype $s echo\""
    set `eval $cmd`
    dispshell=$1
    shift
    shvers=$@
    if [ $rc -eq 0 ]; then
      echo " ok" >&8
      shelllist="${shelllist} $s"
      echo " [$dispshell $shvers] (ok)"
    else
      echo " ng" >&8
      echo " : $chkmsg" >&8
      echo " [$dispshell $shvers] (ng)"
    fi
  done
}

runshelltest () {
  stag=""
  if [ "$_MKCONFIG_SHELL" != "" ]; then
    stag=".${scount}_${dispshell}"
  fi
  TSTRUNLOG=${_MKCONFIG_TSTRUNTMPDIR}/${tbase}.log${stag}
  > $TSTRUNLOG
  exec 9>>$TSTRUNLOG

  echo "####" >&9
  echo "# Test: $tbase $arg" >&9
  if [ "$_MKCONFIG_SHELL" != "" ]; then
    echo "## testing with ${_MKCONFIG_SHELL} " >&9
  fi
  echo "# $dt" >&9
  echo "####" >&9

  cd $_MKCONFIG_TSTRUNTMPDIR
  if [ "$_MKCONFIG_SHELL" != "" ]; then
    putsnonl " ${dispshell}"
  fi
  targ=$arg
  if [ "$arg" != "" ]; then
    targ="$_MKCONFIG_DIR/$arg"
  fi
  # dup stdout to 5; redirect stdout to 9; redirect stderr to new 1.
  ${_MKCONFIG_SHELL} $_MKCONFIG_RUNTESTDIR/$tf "$stag" $targ 5>&1 >&9 2>&1
  rc=$?
  cd $_MKCONFIG_RUNTESTDIR

  dt=`date`
  echo "####" >&9
  echo "# $dt" >&9
  echo "# exit $rc" >&9
  echo "####" >&9
  exec 9>&-
  if [ $rc -ne 0 -a "$_MKCONFIG_SHELL" != "" ]; then
    putsnonl "*"
  fi
  return $rc
}

#
# main
#

_MKCONFIG_RUNTOPDIR=`pwd`
export _MKCONFIG_RUNTOPDIR
mypath=`echo $0 | sed -e 's,/[^/]*$,,' -e 's,^\.,./.,'`
if [ "$mypath" = runtests.sh ]; then
  mypath=.
fi
_MKCONFIG_DIR=`(cd $mypath;pwd)`
export _MKCONFIG_DIR
. ${_MKCONFIG_DIR}/bin/shellfuncs.sh

_MKC_ONCE=0
export _MKC_ONCE
_MKC_SH=1
export _MKC_SH
_MKC_PL=2
export _MKC_PL
_MKC_SH_PL=3
export _MKC_SH_PL

doshelltest $0 $@
if [ $SUBDIR = F ]; then
  mkconfigversion
fi

testdir=$1
if [ ! -d $testdir ]; then
  echo "## Unable to locate $testdir"
  exit 1
fi

shift
teststorun=$*

cd $testdir
if [ $? != 0 ]; then
  echo "## Unable to cd to $testdir"
  exit 1
fi

_MKCONFIG_RUNTESTDIR=`pwd`
export _MKCONFIG_RUNTESTDIR
MKC_FILES=${MKC_FILES:-mkc_files}
export MKC_FILES

if [ $SUBDIR = F ]; then
  _MKCONFIG_RUNTMPDIR=$_MKCONFIG_RUNTOPDIR/${MKC_FILES}/_mkconfig_runtests
  export _MKCONFIG_RUNTMPDIR

  CC=${CC:-cc}
  export CC
  DC=${DC:-gdc}
  export DC
else
  btestdir=`echo $testdir | sed 's,.*/,,'`
  _MKCONFIG_RUNTMPDIR=$_MKCONFIG_RUNTOPDIR/${MKC_FILES}/_mkconfig_runtests/$btestdir
  export _MKCONFIG_RUNTMPDIR
fi
d=`dirname $_MKCONFIG_RUNTMPDIR`
test -d $d || mkdir -p $d

TMPORDER=test_order.tmp
> $TMPORDER
if [ "$teststorun" = "" ]; then
  if [ ! -f "$TESTORDER" ]; then
    ls -1d *.d *.sh 2>/dev/null | sed -e 's/\.sh$//' -e 's/^/1/' >> $TMPORDER
  else
    sort -n $TESTORDER >> $TMPORDER
  fi
else
  for t in $teststorun; do
    echo "1 $t"
  done >> $TMPORDER
fi

if [ $SUBDIR = F ]; then
  test -d $_MKCONFIG_RUNTMPDIR && rm -rf "$_MKCONFIG_RUNTMPDIR"
fi
test -d $_MKCONFIG_RUNTMPDIR || mkdir $_MKCONFIG_RUNTMPDIR

MAINLOG=${_MKCONFIG_RUNTMPDIR}/main.log
if [ $SUBDIR = F ]; then
  > $MAINLOG
fi
exec 8>>$MAINLOG

if [ $SUBDIR = F ]; then
  echo "## locating valid shells"
  getlistofshells
fi

locateawkcmd
echo "awk: $awkcmd" >&8
locatepkgconfigcmd
echo "pkg-config: $pkgconfigcmd" >&8

export shelllist
grc=0
count=0
fcount=0
fpass=0
lastpass=""
# save stdin in fd 7
exec 7<&0 < ${TMPORDER}
while read tline; do
  set $tline
  pass=$1
  tbase=$2
  if [ "$lastpass" = "" ]; then
    lastpass=$pass
  fi
  if [ $grc -ne 0 -a "$lastpass" != "$pass" ]; then
    if [ $SUBDIR = F ]; then
      echo "## stopping tests due to failures in $testdir/ pass $lastpass"
    else
      fpass=$lastpass
    fi
    echo "## stopping tests due to failures in $testdir/ pass $lastpass" >&8
    break
  fi

  if [ -d "$tbase" ]; then
    ocwd=`pwd`
    cd $_MKCONFIG_DIR
    TMPOUT=${_MKCONFIG_RUNTMPDIR}/${tbase}.out
    ${_MKCONFIG_SHELL} ./runtests.sh -s "$CC" "$DC" "$shelllist" "$_pthlist" $TMPOUT $testdir/$tbase
    retvals=`tail -1 $TMPOUT`
    rm -f $TMPOUT > /dev/null 2>&1
    set $retvals
    retcount=$1
    retfcount=$2
    retfpass=$3
    domath count "$count + $retcount"
    domath fcount "$fcount + $retfcount"
    rc=$retfcount
    if [ $rc -ne 0 ]; then
      grc=$rc
      echo "## stopping tests due to failures in $testdir/$tbase/ pass $retfpass (pass $lastpass) "
      echo "## stopping tests due to failures in $testdir/$tbase/ pass $retfpass (pass $lastpass)" >&8
      break
    fi
    cd $ocwd
    continue
  fi

  tprefix=`echo $tbase | sed 's/-.*//'`
  if [ "${CC}" = "" -a "$tprefix" = c ]; then
    continue
  fi
  if [ "${DC}" = "" -a "$tprefix" = d ]; then
    continue
  fi
  tf="${tbase}.sh"
  tconfig="${tbase}.config"
  tconfh="${tbase}.ctmp"

  ok=T
  if [ ! -f ./$tf ]; then
    echo "$tbase ... missing ... failed"
    echo "$tbase ... missing ... failed" >&8
    ok=F
  elif [ ! -x ./$tf ]; then
    echo "$tbase ... permission denied ... failed"
    echo "$tbase ... permission denied ... failed" >&8
    ok=F
  fi
  if [ $ok = F ]; then
    domath fcount "$fcount + 1"
    domath count "$count + 1"
    continue
  fi

  dt=`date`
  arg="mkconfig.sh"

  scount=""
  putsnonl "$tbase ..."
  putsnonl "$tbase ..." >&8
  _MKCONFIG_TSTRUNTMPDIR=$_MKCONFIG_RUNTMPDIR/${tbase}
  export _MKCONFIG_TSTRUNTMPDIR
  mkdir ${_MKCONFIG_TSTRUNTMPDIR}
  if [ -f $tconfig ]; then
    cp $tconfig $_MKCONFIG_TSTRUNTMPDIR/$tconfh
  fi
  tfdisp=`$_MKCONFIG_RUNTESTDIR/$tf -d`
  putsnonl " ${tfdisp}"
  putsnonl " ${tfdisp}" >&8
  $_MKCONFIG_RUNTESTDIR/$tf -q
  runshpl=$?

  if [ $runshpl -eq $_MKC_SH -o $runshpl -eq $_MKC_SH_PL ]; then
    putsnonl " ..."
    putsnonl " ..." >&8
    src=0
    scount=1
    for s in $shelllist; do
      unset _shell
      unset dispshell
      cmd="$s -c \". $_MKCONFIG_DIR/bin/shellfuncs.sh;getshelltype $s echo\""
      set `eval $cmd`
      dispshell=$1
      _MKCONFIG_SHELL=$s
      export _MKCONFIG_SHELL

      runshelltest
      rc=$?
      if [ $rc -ne 0 ]; then src=$rc; fi
      domath scount "$scount + 1"

      unset _shell
      unset dispshell
      unset _MKCONFIG_SHELL
    done
  else
    runshelltest
    src=$?
  fi

  if [ $src -eq 166 ]; then
    echo " ... skipped"
    echo " skipped" >&8
  elif [ $src -ne 0 ]; then
    src=1
    grc=1
    if [ $tbase = c-compiler -a $grc -ne 0 ]; then
      CC=""
      grc=0
    fi
    if [ $tbase = d-compiler -a $grc -ne 0 ]; then
      DC=""
      grc=0
    fi
    if [ $grc -eq 0 ]; then
      echo " ... skipping $tprefix compiler tests"
      echo " skipping $tprefix compiler tests" >&8
    else
      echo " ... failed"
      echo " failed" >&8
      domath fcount "$fcount + 1"
    fi
  else
    echo " ... success"
    echo " success" >&8
  fi
  domath count "$count + 1"

  if [ $src -eq 0 -a "$MKC_CLEAN_RUN_TMP" = Y ]; then
    test -d "$_MKCONFIG_TSTRUNTMPDIR" && rm -rf "$_MKCONFIG_TSTRUNTMPDIR"
  fi

  # for some reason, unixware can't handle this if it is split into
  # multiple lines.
  if [ "$DOPERL" = T -a \( $runshpl -eq $_MKC_PL -o $runshpl -eq $_MKC_SH_PL \) ]; then
    _MKCONFIG_TSTRUNTMPDIR=$_MKCONFIG_RUNTMPDIR/${tbase}_pl
    export _MKCONFIG_TSTRUNTMPDIR
    mkdir ${_MKCONFIG_TSTRUNTMPDIR}
    TSTRUNLOG=$_MKCONFIG_TSTRUNTMPDIR/${tbase}.log
    > $TSTRUNLOG
    exec 9>>$TSTRUNLOG

    dt=`date`
    echo "####" >&9
    echo "# Test: $tf mkconfig.pl" >&9
    echo "# $dt" >&9
    echo "####" >&9
    putsnonl "$tbase ..."
    putsnonl "$tbase ..." >&8
    putsnonl " ${tfdisp}"
    putsnonl " ${tfdisp}" >&8
    putsnonl " ... perl"
    putsnonl " ... perl" >&8
    echo "## Using mkconfig.pl " >&9
    if [ -f $tconfig ]; then
      cp $tconfig $_MKCONFIG_TSTRUNTMPDIR/$tconfh
    fi

    cd $_MKCONFIG_TSTRUNTMPDIR
    # dup stdout to 5; redirect stdout to 9; redirect stderr to new 1.
    $_MKCONFIG_RUNTESTDIR/$tf none $_MKCONFIG_DIR/mkconfig.pl 5>&1 >&9 2>&1
    rc=$?
    cd $_MKCONFIG_RUNTESTDIR

    dt=`date`
    echo "####" >&9
    echo "# $dt" >&9
    echo "# exit $rc" >&9
    echo "####" >&9
    exec 9>&-
    if [ $rc -ne 0 ]; then
      echo " ... failed"
      echo " failed" >&8
      domath fcount "$fcount + 1"
      grc=1
    else
      echo " ... success"
      echo " success" >&8
    fi
    domath count "$count + 1"

    if [ $rc -eq 0 -a "$MKC_CLEAN_RUN_TMP" = Y ]; then
      test -d "$_MKCONFIG_TSTRUNTMPDIR" && rm -rf "$_MKCONFIG_TSTRUNTMPDIR"
    fi
  fi

  lastpass=$pass
done
# set std to saved fd 7; close 7
exec <&7 7<&-
test -f $TMPORDER && rm -f $TMPORDER

if [ $count -eq 0 ]; then  # this can't be right...
  $fcount = -1
fi

exec 8>&-

if [ $SUBDIR = F ]; then
  echo "$count tests $fcount failures"
  if [ $fcount -eq 0 ]; then
    if [ "$MKC_KEEP_RUN_TMP" = "" ]; then
      (
        cd $_MKCONFIG_RUNTMPDIR/../..
        test -d "$_MKCONFIG_RUNTMPDIR" && rm -rf "$_MKCONFIG_RUNTMPDIR"
        test -d "${MKC_FILES}" && rmdir "${MKC_FILES}" > /dev/null 2>&1
      )
    fi
  fi
else
  echo "$count $fcount $fpass" > $TMPOUT
fi

exit $fcount
