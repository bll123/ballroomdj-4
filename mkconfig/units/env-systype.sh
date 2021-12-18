#!/bin/sh
#
# Copyright 2010-2018 Brad Lanam Walnut Creek, CA USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#

#
# speed at the cost of maintainability...
# File Descriptors:
#    9 - >>$LOG                     (mkconfig.sh)
#    8 - >>$VARSFILE, >>$CONFH      (mkconfig.sh)
#    7 - temporary for mkconfig.sh  (mkconfig.sh)
#    6 - temporary for c-main.sh    (c-main.sh)
#    5 - temporary for c-main.sh    (c-main.sh)
#

require_unit env-main

env_dolocuname=F

_dolocuname () {
  if [ $env_dolocuname = "T" ]; then
    return
  fi

  locatecmd xuname uname
  puts "uname located: ${xuname}" >&9
  env_dolocuname=T
}

check_system () {
  name="_MKCONFIG_SYS"
  arg=$2
  uarg=$arg
  toupper uarg
  name="${name}${uarg}"

  _dolocuname

  if [ "$arg" = "type" ]; then
    printlabel $name "system: type"
    checkcache_val ${_MKCONFIG_PREFIX} $name
    if [ $? -eq 0 ]; then return; fi

    if [ "${xuname}" != "" ]
    then
      _MKCONFIG_SYSTYPE=`${xuname} -s`
    else
      puts "no uname, try some guessing" >&9
      # no uname...we'll have to do some guessing.
      _MKCONFIG_SYSTYPE="unknown"
      if [ -f /vmunix ]; then
        # some sort of BSD variant
        _MKCONFIG_SYSTYPE="BSD"
      else
        _MKCONFIG_SYSTYPE="SYSV"      # some SysV variant, probably.
      fi
    fi

    puts "type: ${_MKCONFIG_SYSTYPE}" >&9

    printyesno_val _MKCONFIG_SYSTYPE "${_MKCONFIG_SYSTYPE}"
    setdata ${_MKCONFIG_PREFIX} _MKCONFIG_SYSTYPE "${_MKCONFIG_SYSTYPE}"
  fi

  if [ "$arg" = "rev" ]; then
    printlabel $name "system: rev"
    checkcache_val ${_MKCONFIG_PREFIX} $name
    if [ $? -eq 0 ]; then return; fi

    if [ "${xuname}" != "" ]
    then
      case ${_MKCONFIG_SYSTYPE} in
        AIX)
          tmp=`( (oslevel) 2>/dev/null || puts "not found") 2>&1`
          case "$tmp" in
            'not found')
              _MKCONFIG_SYSREV="$4"."$3"
              ;;
            '<3240'|'<>3240')
              _MKCONFIG_SYSREV=3.2.0
              ;;
            '=3240'|'>3240'|'<3250'|'<>3250')
              _MKCONFIG_SYSREV=3.2.4
              ;;
            '=3250'|'>3250')
              _MKCONFIG_SYSREV=3.2.5
              ;;
            *)
              _MKCONFIG_SYSREV=$tmp
              ;;
            esac
          ;;
        *)
          _MKCONFIG_SYSREV=`${xuname} -r`
          ;;
      esac
    else
      puts "no uname, try some guessing" >&9
      # no uname...we'll have to do some guessing.
      _MKCONFIG_SYSREV="unknown"
      if [ -f /vmunix ]; then
        # sys/param.h might have:
        #   #define BSD 43
        #   #define BSD4_3  1
        rev=`grep '^#define.*BSD[^0-9]' /usr/include/sys/param.h | sed 's,/.*,,'`
        if [ "rev" != "" ]; then
          rev=`puts $rev | sed 's/^[^0-9]*\([0-9]\)\([0-9]\).*/\1.\2/'`
          _MKCONFIG_SYSREV="$rev"
        fi
      fi
    fi

    puts "rev: ${_MKCONFIG_SYSREV}" >&9
    printyesno_val _MKCONFIG_SYSREV "${_MKCONFIG_SYSREV}"
    setdata ${_MKCONFIG_PREFIX} _MKCONFIG_SYSREV "${_MKCONFIG_SYSREV}"
  fi

  if [ "$arg" = "arch" ]; then
    printlabel $name "system: arch"
    checkcache_val ${_MKCONFIG_PREFIX} $name
    if [ $? -eq 0 ]; then return; fi

    if [ "${xuname}" != "" ]
    then
      _MKCONFIG_SYSARCH=`${xuname} -m`
    else
      puts "no uname, try some guessing" >&9
      # no uname...we'll have to do some guessing.
      _MKCONFIG_SYSARCH="unknown"
      locatecmd xarch arch
      puts "arch located: ${xarch}" >&9
      if [ "${xarch}" != "" ]; then
        _MKCONFIG_SYSARCH=`arch`
      fi
    fi

    puts "arch: ${_MKCONFIG_SYSARCH}" >&9

    printyesno_val _MKCONFIG_SYSARCH "${_MKCONFIG_SYSARCH}"
    setdata ${_MKCONFIG_PREFIX} _MKCONFIG_SYSARCH "${_MKCONFIG_SYSARCH}"
  fi
}

check_standard_system () {
  check_system system type
  check_system system rev
  check_system system arch
}
