#!/bin/sh
#
# Copyright 2009-2018 Brad Lanam Walnut Creek, CA USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#

#
# speed at the cost of maintainability...
# File Descriptors:
#    9 - >>$LOG                 (mkconfig.sh)
#    8 - >>$VARSFILE, >>$CONFH  (mkconfig.sh)
#    7 - saved stdin            (mkconfig.sh)
#    6 - temporary              (c-main.sh, mkconfig.sh)
#    4 - temporary              (c-main.sh)
#

# disable path name expansion
set -f  # set this globally.

unset CDPATH
# this is a workaround for ksh93 on solaris
if [ "$1" = -d ]; then
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

MKC_FILES=${MKC_FILES:-mkc_files}
LOG="${MKC_FILES}/mkconfig.log"
_MKCONFIG_TMP="${MKC_FILES}/_tmp_mkconfig"
CACHEFILE="${MKC_FILES}/mkconfig.cache"
OPTIONFILE="options.dat"
_MKCONFIG_PREFIX=mkc    # need a default in case no units loaded
optionsloaded=F
allchglist=""

INC="mkcinclude.txt"                   # temporary

_chkconfigfname () {
  if [ "$CONFH" = "" ]; then
    puts "Config file name not set.  Exiting."
    _exitmkconfig 1
  fi
}

_exitmkconfig () {
    rc=$1
    exit $rc
}

_savecache () {
    # And save the data for re-use.
    # Some shells don't quote the values in the set
    # command like bash does.  So we do it.
    # Then we have to undo it for bash.
    #
    # The $'' syntax interpolates backslashes and nothing else.
    # For these, remove the $.
    #
    # And then there's: x='', which gets munged.
    # Any value that actually ends with an '=' is going to get mangled.
    #
    #savecachedebug=F
    #if [ $savecachedebug = T ]; then
    #  puts "## savecache original"
    #  set | grep "^mkc_"
    #  puts "## savecache A"
    #  set | grep "^mkc_" | \
    #    sed -e "s/=/='/"
    #  puts "## savecache B"
    #  set | grep "^mkc_" | \
    #    sed -e "s/=/='/" -e "s/$/'/"
    #  puts "## savecache C"
    #  set | grep "^mkc_" | \
    #    sed -e "s/=/='/" -e "s/$/'/" -e "s/''/'/g"
    #  puts "## savecache D"
    #  set | grep "^mkc_" | \
    #    sed -e "s/=/='/" -e "s/$/'/" -e "s/''/'/g" \
    #    -e "s/^\([^=]*\)='$/\1=''/"
    #  puts "## savecache E"
    #  set | grep "^mkc_" | \
    #    sed -e "s/=/='/" -e "s/$/'/" -e "s/''/'/g" \
    #    -e "s/^\([^=]*\)='$/\1=''/" -e "s/='\$'/='/"
    #fi
    #unset savecachedebug
    set | grep "^mkc_" | \
      sed -e "s/=/='/" -e "s/$/'/" -e "s/''/'/g" \
      -e "s/^\([^=]*\)='$/\1=''/" -e "s/='\$'/='/" \
      > ${CACHEFILE}
}

setvariable () {
    prefix=$1
    svname=$2

    if [ $varsfileopen = F ]; then
      for v in `set | grep "^mkv_" | sed -e 's/=.*$//'`; do
        unset $v
      done
      varsfileopen=T
      >$VARSFILE
      exec 8>>$VARSFILE
    fi

    cmd="test \"X\$mkv_${prefix}_${svname}\" != X > /dev/null 2>&1"
    eval $cmd
    rc=$?
    # if already in the list of vars, don't add it to the file again.
    if [ $rc -ne 0 ]; then
      puts ${svname} >&8
    fi

    cmd="mkv_${prefix}_${svname}=T"
    eval $cmd
    puts "   setvariable: $cmd" >&9
}

setdata () {
    prefix=$1
    sdname=$2
    sdval=$3

    if [ "$_MKCONFIG_EXPORT" = T ]; then
      _doexport $sdname "$sdval"
    fi

    cmd="mkc_${prefix}_${sdname}=\"${sdval}\""
    eval $cmd
    puts "   setdata: $cmd" >&9
    setvariable $prefix $sdname
}

getdata () {
    var=$1
    prefix=$2
    gdname=$3

    cmd="${var}=\${mkc_${prefix}_${gdname}}"
    eval $cmd
}

_setifleveldisp () {
  ifleveldisp=""
  for il in $iflevels; do
    ifleveldisp="${il}${ifleveldisp}"
  done
  if [ "$ifleveldisp" != "" ]; then
    doappend ifleveldisp " "
  fi
}

printlabel () {
  tname=$1
  tlabel=$2

  puts "   $ifleveldisp[${tname}] ${tlabel} ... " >&9
  putsnonl "${ifleveldisp}${tlabel} ..." >&1
}

_doexport () {
  var=$1
  val=$2

  cmd="${var}=\"${val}\""
  eval $cmd
  cmd="export ${var}"
  eval $cmd
}

printyesno_actual () {
  ynname=$1
  ynval=$2
  yntag=${3:-}

  puts "   [${ynname}] $ynval ${yntag}" >&9
  puts " $ynval ${yntag}" >&1
}

printyesno_val () {
  ynname=$1
  ynval=$2
  yntag=${3:-}

  if [ "$ynval" != 0 ]; then
    printyesno_actual "$ynname" "$ynval" "${yntag}"
  else
    printyesno_actual "$ynname" no "${yntag}"
  fi
}

printyesno () {
    ynname=$1
    ynval=$2
    yntag=${3:-}

    if [ "$ynval" != 0 ]; then
      ynval=yes
    fi
    printyesno_val "$ynname" $ynval "$yntag"
}

checkcache_val () {
  prefix=$1
  tname=$2

  getdata tval ${prefix} ${tname}
  rc=1
  if [ "$tval" != "" ]; then
    setvariable $prefix $tname
    printyesno_actual $tname "$tval" " (cached)"
    rc=0
  fi
  return $rc
}

checkcache () {
  prefix=$1
  tname=$2

  getdata tval ${prefix} ${tname}
  rc=1
  if [ "$tval" != "" ]; then
    setvariable $prefix $tname
    printyesno $tname $tval " (cached)"
    rc=0
  fi
  return $rc
}

_loadoptions () {
  if [ $optionsloaded = F -a -f "${OPTIONFILE}" ]; then
    exec 6<&0 < ${OPTIONFILE}
    while read o; do
      case $o in
        "")
          continue
          ;;
        \#*)
          continue
          ;;
      esac

      topt=`puts "$o" | sed 's/=.*//'`
      tval=`puts "$o" | sed 's/.*=//'`
      eval "_mkc_opt_${topt}=\"${tval}\""
    done
    exec <&6 6<&-
    optionsloaded=T
  fi
}

check_command () {
    name=$1
    shift
    ccmd=$1

    locnm=_cmd_loc_${ccmd}

    printlabel $name "command: ${ccmd}"
    checkcache ${_MKCONFIG_PREFIX} $name
    if [ $rc -eq 0 ]; then return; fi

    trc=0
    val=""
    while test $# -gt 0; do
      ccmd=$1
      shift
      locatecmd tval $ccmd
      if [ "$tval" != "" ]; then
        val=$tval
        trc=1
      fi
    done

    printyesno_val $name $val
    setdata ${_MKCONFIG_PREFIX} ${name} ${trc}
    if [ $trc -eq 1 ]; then
      setdata ${_MKCONFIG_PREFIX} ${locnm} ${val}
    fi
}

check_grep () {
  name=$1; shift
  tag=$1; shift
  pat=$1; shift
  fn=$1; shift

  locnm=_grep_${name}

  printlabel $name "grep: ${tag}"
  checkcache ${_MKCONFIG_PREFIX} $name
  if [ $rc -eq 0 ]; then return; fi

  grep -l ${pat} ${fn} > /dev/null 2>&1
  rc=$?
  if [ $rc -eq 0 ]; then trc=1; else trc=0; fi

  printyesno $name $trc
  setdata ${_MKCONFIG_PREFIX} ${name} ${trc}
}

check_ifoption () {
    ifdispcount=$1
    type=$2
    name=$3
    oopt=$4

    printlabel $name "$type ($ifdispcount): ${oopt}"

    _loadoptions
    trc=0  # if option is not set, it's false

    found=F
    if [ $optionsloaded = T ]; then
      eval tval=\$_mkc_opt_${oopt}
      # override the option value with the environment variable
      eval tenvval=\$${oopt}
      if [ "$tenvval" != "" ]; then
        tval=$tenvval
      fi
      if [ "$tval" != "" ]; then
        found=T
        trc=$tval
        tolower trc
        puts "  found option: $oopt $trc" >&9
        if [ "$trc" = t ]; then trc=1; fi
        if [ "$trc" = f ]; then trc=0; fi
        if [ "$trc" = enable ]; then trc=1; fi
        if [ "$trc" = disable ]; then trc=0; fi
        if [ "$trc" = true ]; then trc=1; fi
        if [ "$trc" = false ]; then trc=0; fi
        if [ "$trc" = yes ]; then trc=1; fi
        if [ "$trc" = no ]; then trc=0; fi
      fi
    fi

    # these must be set before ifnotoption processing
    if [ $optionsloaded = F ]; then
      trc=0
    elif [ "$found" = F ]; then
      trc=0
    fi

    if [ $type = ifnotoption ]; then
      if [ $trc -eq 0 ]; then trc=1; else trc=0; fi
    fi

    if [ $optionsloaded = F ]; then
      printyesno_actual $name "no options file"
    elif [ "$found" = F ]; then
      printyesno_actual $name "option not found"
    else
      printyesno $name $trc
    fi
    return $trc
}

check_if () {
    iflabel=$1
    ifdispcount=$2
    ifline=$3

    name=$iflabel
    printlabel $name, "if ($ifdispcount): $iflabel";

    boolclean ifline
    puts "## ifline: $ifline" >&9

    trc=0  # if option is not set, it's false

    nline="test "
    ineq=0
    qtoken=""
    quoted=0
    for token in $ifline; do
      puts "## token: $token" >&9

      case $token in
        \'*\')
          token=`puts $token | sed -e s,\',,g`
          puts "## begin/end quoted token" >&9
          ;;
        \'*)
          qtoken=$token
          puts "## begin qtoken: $qtoken" >&9
          quoted=1
          continue
          ;;
      esac

      if [ $quoted -eq 1 ]; then
        case $token in
          *\')
            token="${qtoken} $token"
            token=`puts $token | sed -e s,\',,g`
            puts "## end qtoken: $token" >&9
            quoted=0
            ;;
          *)
            qtoken="$qtoken $token"
            puts "## in qtoken: $qtoken" >&9
            continue
            ;;
        esac
      fi

      if [ $ineq -eq 1 ]; then
        ineq=2
        getdata tvar ${_MKCONFIG_PREFIX} $token
      elif [ $ineq -eq 2 ]; then
        doappend nline " ( '$tvar' = '$token' )"
        ineq=0
      else
        case $token in
          ==)
            ineq=1
            ;;
          \(|\)|-a|-o|!)
            doappend nline " $token"
            ;;
          *)
            getdata tvar ${_MKCONFIG_PREFIX} $token
            if [ "$tvar" != 0 -a "$tvar" != "" ]; then tvar=1; else tvar=0; fi
            tvar="( $tvar = 1 )"
            doappend nline " $tvar"
          ;;
        esac
      fi
    done

    if [ "$ifline" != "" ]; then
      dosubst nline '(' '\\\\\\(' ')' '\\\\\\)'
      puts "## nline: $nline" >&9
      eval $nline
      trc=$?
      puts "## eval nline: $trc" >&9
      # replace w/ shell return
      if [ $trc -eq 0 ]; then trc=1; else trc=0; fi
      puts "## eval nline final: $trc" >&9
    fi

    texp=$_MKCONFIG_EXPORT
    _MKCONFIG_EXPORT=F
    printyesno "$name" $trc
    _MKCONFIG_EXPORT=$texp
    return $trc
}

check_set () {
  nm=$1
  type=$2
  sval=$3

  name=$type
  tnm=$1
  dosubst tnm '_setint_' '' '_setstr' ''

  printlabel $name "${type}: ${tnm}"
  if [ "$type" = set ]; then
    getdata tval ${prefix} ${nm}
    if [ "$tval" != "" ]; then
      printyesno $nm "${sval}"
      setdata ${_MKCONFIG_PREFIX} ${nm} "${sval}"
    else
      printyesno_actual $nm "no such variable"
    fi
  elif [ "$type" = setint ]; then
    printyesno_actual $nm "${sval}"
    setdata ${_MKCONFIG_PREFIX} ${nm} "${sval}"
  else
    printyesno_actual $nm "${sval}"
    setdata ${_MKCONFIG_PREFIX} ${nm} "${sval}"
  fi
}

_read_option () {
  onm=$1
  def=$2

  _loadoptions

  oval=$def
  if [ $optionsloaded = T ]; then
    eval tval=\$_mkc_opt_${onm}
    if [ "$tval" != "" ]; then
      found=T
      puts "  found option: $onm $tval" >&9
      oval="$tval"
    fi
  fi
}

check_option () {
  nm=$1
  onm=$2
  def=$3

  name=$nm

  printlabel $name "option: ${onm}"

  _read_option $onm "$def"

  printyesno_actual $nm "$oval"
  setdata ${_MKCONFIG_PREFIX} ${nm} "${oval}"
}

check_echo () {
  val=$1

  puts "## echo: $val" >&9
  puts "$val" >&1
}

check_exit () {
  puts "## exit" >&9
  _exitmkconfig 5
}

check_substitute () {
  nm=$1
  sub1=$2
  sub2=$3

  printlabel $nm "substitute: ${sub1} ${sub2}"
  doappend allchglist " -e 's~${sub1}~${sub2}~g'"
  printyesno $nm 1
}

_doloadunit () {
  lu=$1
  dep=$2
  if [ "$dep" = Y ]; then
   slu=${lu}
   tag=" (dependency)"
  fi
  if [ -f ${_MKCONFIG_DIR}/units/${lu}.sh ]; then
    puts "load-unit: ${lu} ${tag}" >&1
    puts "   found ${lu} ${tag}" >&9
    . ${_MKCONFIG_DIR}/units/${lu}.sh
    tlu=$lu
    dosubst tlu '-' '_'
    eval "_MKCONFIG_UNIT_${tlu}=Y"
  fi
  if [ "$dep" = Y ]; then
    lu=$slu
    tag=""
  fi
}

require_unit () {
  units=$@
  for rqu in $units; do
    trqu=$rqu
    dosubst trqu '-' '_'
    cmd="val=\$_MKCONFIG_UNIT_${trqu}"
    eval $cmd
    if [ "$val" = Y ]; then
      puts "   required unit ${rqu} already loaded" >&9
      continue
    fi
    puts "   required unit ${rqu} needed" >&9
    _doloadunit $rqu Y
  done
}

_create_output () {

  if [ ${CONFH} != none ]; then
    confdir=`puts ${CONFH} | sed -e 's,/[^/]*$,,'`
    test -d $confdir || mkdir -p $confdir

    > ${CONFH}
    exec 8>>${CONFH}
    preconfigfile ${CONFH} ${configfile} >&8

    if [ -f $VARSFILE ]; then
      exec 6<&0 < $VARSFILE
      while read cfgvar; do
        getdata val ${_MKCONFIG_PREFIX} $cfgvar
        output_item ${CONFH} ${cfgvar} "${val}" >&8
      done
      exec <&6 6<&-
    fi

    stdconfigfile ${CONFH} >&8
    cat $INC >&8
    postconfigfile ${CONFH} >&8
    exec 8>&-
    if [ "$allchglist" != "" ]; then
      cmd="sed ${allchglist} ${CONFH} > ${CONFH}.tmp;mv ${CONFH}.tmp ${CONFH}"
      eval $cmd
    fi
  fi
}

main_process () {
  configfile=$1

  reqlibs=""

  if [ -f "$CACHEFILE" ]; then
    . $CACHEFILE
  fi

  reqhdr=""

  inproc=0
  ininclude=0
  doproclist=""
  doproc=1
  linenumber=0
  ifstmtcount=0
  ifleveldisp=""
  iflevels=""
  ifcurrlvl=0
  doif=$ifcurrlvl
  initifs
  > $INC
  case ${configfile} in
    /*)
      ;;
    *)
      configfile="../../${configfile}"
      ;;
  esac
  # save stdin in fd 7.
  # and reset stdin to get from the configfile.
  # this allows us to run the while loop in the
  # current shell rather than a subshell.

  # default varsfile.
  # a main loadunit will override this.
  # but don't open it unless it is needed.
  varsfileopen=F
  if [ "$VARSFILE" = "" -a "${_MKCONFIG_PREFIX}" != "" ]; then
    VARSFILE="../mkc_none_${_MKCONFIG_PREFIX}.vars"
  fi

  # ksh93 93u 'read' changed.  Need the raw read for 'include'.
  # Unfortunately, this affects other shells.
  # shellfuncs tests for the necessity.
  rawarg=
  # save stdin in fd 7; open stdin
  exec 7<&0 < ${configfile}
  while read ${rawarg} tdatline; do
    resetifs
    domath linenumber "$linenumber + 1"
    puts "#### ${linenumber}: ${tdatline}" >&9

    if [ $ininclude -eq 1 ]; then
      if [ "${tdatline}" = endinclude ]; then
        ininclude=0
        rawarg=
        resetifs
      else
        if [ $shreqreadraw -eq 1 ]; then
          # have to do our own backslash processing.
          # backquotes suck.
          tdatline=`puts "${tdatline}" |
              sed -e 's/\\\\\\([^\\\\]\\)/\\1/g' -e 's/\\\\\\\\/\\\\/g'`
        fi
        puts "${tdatline}" >> $INC
      fi
    else
      case ${tdatline} in
        "")
          continue
          ;;
        \#*)
          continue
          ;;
        *)
          puts "#### ${linenumber}: ${tdatline}" >&9
          ;;
      esac
    fi

    if [ $ininclude -eq 0 ]; then
      case ${tdatline} in
        "else")
          if [ $ifcurrlvl -eq $doif ]; then
            if [ $doproc -eq 0 ]; then doproc=1; else doproc=0; fi
            set -- $iflevels
            shift
            iflevels=$@
            iflevels="-$ifstmtcount $iflevels"
            _setifleveldisp
            puts "## else: ifcurrlvl: $ifcurrlvl doif: $doif doproc:$doproc" >&9
            puts "## else: iflevels: $iflevels" >&9
          else
            puts "## else: ifcurrlvl: $ifcurrlvl doif: $doif doproc:$doproc" >&9
          fi
          ;;
        "if "*|"ifoption"*|"ifnotoption"*)
          if [ $doproc -eq 0 ]; then
            domath ifcurrlvl "$ifcurrlvl + 1"
            puts "## if: ifcurrlvl: $ifcurrlvl doif: $doif" >&9
          fi
          ;;
        "endif")
          puts "## endifA: ifcurrlvl: $ifcurrlvl doif: $doif" >&9
          if [ $ifcurrlvl -eq $doif ]; then
            set $doproclist
            c=$#
            if [ $c -gt 0 ]; then
              puts "## doproclist: $doproclist" >&9
              doproc=$1
              shift
              doproclist=$@
              puts "## doproc: $doproc doproclist: $doproclist" >&9
              set -- $iflevels
              shift
              iflevels=$@
              _setifleveldisp
              puts "## endif iflevels: $iflevels" >&9
            else
              doproc=1
              ifleveldisp=""
              iflevels=""
            fi
            domath doif "$doif - 1"
          fi
          domath ifcurrlvl "$ifcurrlvl - 1"
          puts "## endifB: ifcurrlvl: $ifcurrlvl doif: $doif" >&9
          ;;
      esac

      if [ $doproc -eq 1 ]; then
        case ${tdatline} in
          command*)
            _chkconfigfname
            set $tdatline
            cmd=$2
            shift
            nm="_command_${cmd}"
            check_command ${nm} $@
            ;;
          grep*)
            _chkconfigfname
            set $tdatline
            tag=$2
            shift
            nm="_grep_${tag}"
            check_grep ${nm} $@
            ;;
          "echo"*)
            _chkconfigfname
            set $tdatline
            shift
            val=$@
            check_echo "${val}"
            ;;
          "exit")
            check_exit
            ;;
          substitute)
            check_substitute $@
            ;;
          endinclude)
            ;;
          ifoption*|ifnotoption*)
            _chkconfigfname
            set $tdatline
            type=$1
            opt=$2
            nm="_${type}_${opt}"
            puts "## if: ifcurrlvl: $ifcurrlvl" >&9
            domath ifstmtcount "$ifstmtcount + 1"
            check_ifoption $ifstmtcount $type ${nm} ${opt}
            rc=$?
            iflevels="+$ifstmtcount $iflevels"
            _setifleveldisp
            puts "## ifopt iflevels: $iflevels" >&9
            doproclist="$doproc $doproclist"
            doproc=$rc
            puts "## doproc: $doproc doproclist: $doproclist" >&9
            ;;
          "if "*)
            _chkconfigfname
            set $tdatline
            shift
            label=$1
            shift
            ifline=$@
            domath ifcurrlvl "$ifcurrlvl + 1"
            puts "## if: ifcurrlvl: $ifcurrlvl" >&9
            domath ifstmtcount "$ifstmtcount + 1"
            check_if $label $ifstmtcount "$ifline"
            rc=$?
            iflevels="+$ifstmtcount $iflevels"
            _setifleveldisp
            puts "## if iflevels: $iflevels" >&9
            doproclist="$doproc $doproclist"
            doproc=$rc
            doif=$ifcurrlvl
            puts "## doproc: $doproc doproclist: $doproclist" >&9
            ;;
          "else")
            ;;
          "endif")
            ;;
          include)
            _chkconfigfname
            ininclude=1
            if [ $shreqreadraw -eq 1 ]; then
              rawarg=-r
            fi
            ;;
          loadunit*)
            set $tdatline
            type=$1
            file=$2
            _doloadunit ${file} N
            if [ $varsfileopen = T ]; then
              exec 8>&-
              varsfileopen=F
            fi
            if [ "${_MKCONFIG_PREFIX}" != "" ]; then
              VARSFILE="../mkc_${CONFHTAG}_${_MKCONFIG_PREFIX}.vars"
            fi
            ;;
          option-file*)
            set $tdatline
            type=$1
            file=$2
            case ${file} in
              /*)
                OPTIONFILE=${file}
                ;;
              *)
                OPTIONFILE="../../${file}"
                ;;
            esac
            puts "option-file: ${file}" >&1
            puts "   option file name: ${OPTIONFILE}" >&9
            ;;
          option*)
            _chkconfigfname
            set $tdatline
            optnm=$2
            shift; shift
            tval=$@
            nm="_opt_${optnm}"
            check_option ${nm} $optnm "${tval}"
            ;;
          output*)
            newout=F
            if [ $varsfileopen = T ]; then
              exec 8>&-
              varsfileopen=F
              newout=T
            fi
            if [ $inproc -eq 1 ]; then
              _create_output
              CONFH=none
              CONFHTAG=none
              CONFHTAGUC=NONE
            fi
            if [ $newout = T ]; then
              new_output_file
              > $INC  # restart include
            fi
            set $tdatline
            type=$1
            file=$2
            case ${file} in
              none)
                CONFH=${file}
                ;;
              /*)
                CONFH=${file}
                ;;
              *)
                CONFH="../../${file}"
                ;;
            esac
            puts "output-file: ${file}" >&1
            puts "   config file name: ${CONFH}" >&9
            file=`puts $file | sed -e 's,.*/,,'`
            file=`puts $file | sed -e 's/\..*//'`
            CONFHTAG=$file
            CONFHTAGUC=$file
            toupper CONFHTAGUC
            if [ "${_MKCONFIG_PREFIX}" != "" ]; then
              VARSFILE="../mkc_${CONFHTAG}_${_MKCONFIG_PREFIX}.vars"
            fi
            inproc=1
            ;;
          standard)
            _chkconfigfname
            standard_checks
            ;;
          "set "*|setint*|setstr*)
            _chkconfigfname
            set $tdatline
            type=$1
            nm=$2
            if [ "$type" = setint -o "$type" = setstr ]; then
              nm="_${type}_$2"
            fi
            shift; shift
            tval=$@
            check_set ${nm} $type "${tval}"
            ;;
          *)
            _chkconfigfname
            set $tdatline
            type=$1
            chk="check_${type}"
            cmd="$chk $@"
            eval $cmd
            ;;
        esac
      fi  # doproc
    fi # ininclude
    if [ $ininclude -eq 1 ]; then
      setifs
    fi
  done
  # reset the file descriptors back to the norm.
  # set stdin to saved fd 7; close fd 7
  exec <&7 7<&-
  if [ $varsfileopen = T ]; then
    exec 8>&-
    varsfileopen=F
  fi

  _savecache     # save the cache file.
  _create_output
}

usage () {
  puts "Usage: $0 [-C] [-c <cache-file>] [-o <options-file>]
           [-L <log-file>] <config-file>
  -C : clear cache-file
defaults:
  <cache-file> : mkconfig.cache
  <log-file>   : mkconfig.log"
}

# main

mkconfigversion

unset GREP_OPTIONS
unset ENV
unalias sed > /dev/null 2>&1
unalias grep > /dev/null 2>&1
unalias ls > /dev/null 2>&1
unalias rm > /dev/null 2>&1
LC_ALL=C
export LC_ALL
clearcache=0
while test $# -gt 1; do
  case $1 in
    -C)
      shift
      clearcache=1
      ;;
    -c)
      shift
      CACHEFILE=$1
      shift
      ;;
    -L)
      shift
      LOG=$1
      shift
      ;;
    -o)
      shift
      OPTIONFILE=$1
      shift
      ;;
  esac
done

configfile=$1
if [ $# -ne 1 ] || [ ! -f $configfile  ]; then
  puts "No configuration file specified or not found."
  usage
  exit 1
fi
if [ -d $_MKCONFIG_TMP -a $_MKCONFIG_TMP != ${MKC_FILES}/_tmp_mkconfig ]; then
  puts "$_MKCONFIG_TMP must not exist."
  usage
  exit 1
fi

test -d $_MKCONFIG_TMP && rm -rf $_MKCONFIG_TMP > /dev/null 2>&1
mkdir -p $_MKCONFIG_TMP
cd $_MKCONFIG_TMP

LOG="../../$LOG"
REQLIB="../../$REQLIB"
CACHEFILE="../../$CACHEFILE"
VARSFILE="../../$VARSFILE"
OPTIONFILE="../../$OPTIONFILE"
CONFH=none
CONFHTAG=none
CONFHTAGUC=NONE

if [ $clearcache -eq 1 ]; then
  rm -f $CACHEFILE > /dev/null 2>&1
  rm -f ../mkconfig_*.vars > /dev/null 2>&1
fi

dt=`date`
exec 9>>$LOG

puts "#### " >&9
puts "# Start: $dt " >&9
puts "# $0 ($shell) using $configfile " >&9
puts "#### " >&9
puts "shell: $shell" >&9
puts "has printf: ${shhasprintf}" >&9
puts "has append: ${shhasappend}" >&9
puts "has math: ${shhasmath}" >&9
puts "has upper: ${shhasupper}" >&9
puts "has lower: ${shhaslower}" >&9
puts "read raw req: ${shreqreadraw}" >&9

locateawkcmd
puts "awk: $awkcmd" >&9
locatepkgconfigcmd
echo "pkg-config: $pkgconfigcmd" >&9

puts "$0 ($shell) using $configfile"

main_process $configfile

dt=`date`
puts "#### " >&9
puts "# End: $dt " >&9
puts "#### " >&9
exec 9>&-

cd ..

if [ "$MKC_KEEP_TMP" = "" ]; then
  test -d $_MKCONFIG_TMP && rm -rf $_MKCONFIG_TMP > /dev/null 2>&1
fi
exit 0
