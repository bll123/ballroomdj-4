#!/bin/sh
#
# Copyright 2010-2018 Brad Lanam Walnut Creek CA USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#
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

_MKCONFIG_PREFIX=env
_MKCONFIG_HASEMPTY=T
_MKCONFIG_EXPORT=T

preconfigfile () {
  pc_configfile=$1
  configfile=$2

  puts "#!/bin/sh"
  puts "# Created on: `date`"
  puts "#  From: ${configfile}"
  puts "#  Using: mkconfig-${_MKCONFIG_VERSION}"
  return
}

stdconfigfile () {
  pc_configfile=$1
  return
}

postconfigfile () {
  pc_configfile=$1
  return
}

standard_checks () {
  return
}

check_source () {
  nm=$1
  fn=$2

  tfn=$fn
  dosubst tfn '.*/' '' '\.' '' '-' ''
  name=_${nm}_${tfn}

  printlabel $nm "source: $fn"

  trc=0
  val=0
  if [ -f $fn ]; then
    trc=1
    val=$fn
  fi
  printyesno $name $trc
  setdata ${_MKCONFIG_PREFIX} ${name} $val
}

output_item () {
  out=$1
  name=$2
  val=$3

  case $name in
    _source_*)
      if [ $val != "0" ]; then
        puts ". ${val}"
      fi
      ;;
    _setint*|_setstr*|_opt_*)
      tname=$name
      dosubst tname '_setint_' '' '_setstr_' '' '_opt_' ''
      puts "${tname}=\"${val}\""
      puts "export ${tname}"
      ;;

    *)
      puts "${name}=\"${val}\""
      puts "export ${name}"
      ;;
  esac
}

check_test_multword () {
  name=$1

  printlabel $name "test: multiword"
  checkcache_val ${_MKCONFIG_PREFIX} $name
  if [ $? -eq 0 ]; then return; fi
  val="word1 word2"
  printyesno_val $name "$val"
  setdata ${_MKCONFIG_PREFIX} $name "$val"
}

new_output_file () {
  return 0
}

check_pkg () {
  type=$2
  pkgname=$3
  pkgpath=$4
  name=_pkg_${type}_${pkgname}
  dosubst name '\.' '' '-' ''

  puts "pkg $type: $pkgname $pkgpath" >&9
  printlabel $name "pkg ${type}: ${pkgname}"

  check_pkg_exists $name $pkgname $pkgpath
  trc=$?
  if [ $trc -eq 0 ]; then
    return $trc
  fi
  check_pkg_$type $name $pkgname $pkgpath
  trc=$?
  return $trc
}

check_pkg_exists () {
  name=$1
  pkgname=$2
  pkgpath=$3

  trc=0
  if [ "${pkgconfigcmd}" = "" ]; then
    printyesno $name $trc "(no pkg-config)"
    return $trc
  fi

  OPKG_CONFIG_PATH=$PKG_CONFIG_PATH
  if [ "$pkgpath" != "" ]; then
    if [ "$PKG_CONFIG_PATH" != "" ]; then
      doappend PKG_CONFIG_PATH :
    fi
    doappend PKG_CONFIG_PATH $pkgpath
  fi
  export PKG_CONFIG_PATH
  ${pkgconfigcmd} --exists $pkgname
  rc=$?
  if [ $rc -ne 0 ]; then
    printyesno $name $trc "(no such package)"
  else
    trc=1
  fi

  unset PKG_CONFIG_PATH
  if [ "$OPKG_CONFIG_PATH" != "" ]; then
    PKG_CONFIG_PATH=$OPKG_CONFIG_PATH
  fi

  return $trc
}

test_cflag () {
  flag=$1

  puts "#include <stdio.h>
int main () { return 0; }" > t.c
  puts "# test ${flag}" >&9
  # need to set w/all cflags; gcc doesn't always error out otherwise
  TMPF=t$$.txt
  setcflags
  ${CC} ${CFLAGS} ${flag} t.c > $TMPF 2>&1
  rc=$?
  if [ $rc -ne 0 ]; then
    flag=0
  fi
  grep -i "warning.*${flag}" $TMPF > /dev/null 2>&1
  rc=$?
  if [ $rc -eq 0 ]; then
    flag=0
  fi
  cat $TMPF >&9
  rm -f $TMPF > /dev/null 2>&1
}

test_ldflags () {
  flag=$1

  setcflags
  setldflags
  setlibs
  puts "#include <stdio.h>
int main () { return 0; }" > t.c
  puts "# test ${flag}" >&9
  # need to set w/all cflags/ldflags; gcc doesn't always error out otherwise
  TMPF=t$$.txt
  ${CC} ${CFLAGS} ${LDFLAGS} ${flag} -o t t.c > $TMPF 2>&1
  rc=$?
  if [ $rc -ne 0 ]; then
    flag=0
  fi
  grep -i "warning.*${flag}" $TMPF > /dev/null 2>&1
  rc=$?
  if [ $rc -eq 0 ]; then
    flag=0
  fi
  cat $TMPF >&9
  rm -f $TMPF > /dev/null 2>&1
}
