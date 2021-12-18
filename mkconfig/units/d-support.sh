#!/bin/sh
#
# Copyright 2010-2018 Brad Lanam Walnut Creek CA USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#
#

_d_print_imports () {
  imports=$1

  if [ "$imports" = "none" ]; then
    return
  fi

  out="${PI_PREFIX}${imports}"

  if [ -f $out ]; then
    cat $out
    return
  fi

  if [ "$PI_ALL" = "T" -a "$imports" = "all" ]; then
    _d_print_imps all > $out
    cat $out
    return
  fi

  # until PI_ALL becomes true, just do normal processing.
  _d_print_imps $imports
}

_d_print_imps () {
  imports=$1

  if [ "${imports}" = "all" -o "${imports}" = "std" ]; then
      for tnm in '_import_std_stdio' '_import_std_string'; do
          getdata tval ${_MKCONFIG_PREFIX} ${tnm}
          if [ "${tval}" != "0" -a "${tval}" != "" ]; then
              puts "import ${tval};"
          fi
      done
  fi

  if [ "${imports}" = "all" -a -f "$VARSFILE" ]; then
    # save stdin in fd 6; open stdin
    exec 6<&0 < ${VARSFILE}
    while read cfgvar; do
      getdata hdval ${_MKCONFIG_PREFIX} ${cfgvar}
      case ${cfgvar} in
        _import_std_stdio|_import_std_string)
            ;;
        _import_*)
            if [ "${hdval}" != "0" -a "${hdval}" != "" ]; then
              puts "import ${hdval};"
            fi
            ;;
      esac
    done
    # set std to saved fd 6; close 6
    exec <&6 6<&-
  fi
}

_d_chk_run () {
  drname=$1
  code=$2
  inc=$3

  _d_chk_link_libs ${drname} "${code}" $inc
  rc=$?
  puts "##  run test: link: $rc" >&9
  rval=0
  if [ $rc -eq 0 ]; then
      rval=`./${drname}.exe`
      rc=$?
      puts "##  run test: run: $rc retval: $rval" >&9
      if [ $rc -lt 0 ]; then
          _exitmkconfig $rc
      fi
  fi
  _retval=$rval
  return $rc
}

_d_chk_link_libs () {
  dllname=$1
  code=$2
  inc=$3
  shift;shift;shift

  ocounter=0
  dlotherlibs="'$otherlibs'"
  dosubst dlotherlibs ',' "' '"
  if [ "${dlotherlibs}" != "" ]; then
    eval "set -- $dlotherlibs"
    ocount=$#
  else
    ocount=0
  fi

  tdfile=${dllname}.d
  # $dllname should be unique
  exec 4>>${tdfile}
  _d_print_imports $inc >&4
  puts "${code}" | sed 's/_dollar_/$/g' >&4
  exec 4>&-

  dlibs=""
  otherlibs=""
  _d_chk_link $dllname
  rc=$?
  puts "##      link test (none): $rc" >&9
  if [ $rc -ne 0 ]; then
    while test $ocounter -lt $ocount; do
      domath ocounter "$ocounter + 1"
      eval "set -- $dlotherlibs"
      cmd="olibs=\$${ocounter}"
      eval $cmd
      dlibs=${olibs}
      otherlibs=${olibs}
      _d_chk_link $dllname
      rc=$?
      puts "##      link test (${olibs}): $rc" >&9
      if [ $rc -eq 0 ]; then
          break
      fi
    done
  fi
  _retdlibs=$dlibs
  return $rc
}

_d_chk_link () {
  dlname=$1

  cmd="${_MKCONFIG_DIR}/mkc.sh -d `pwd` -complink -e -c ${DC} \
      -o ${dlname}${OBJ_EXT} -- ${DFLAGS} ${dlname}.d "
  puts "##  _link test (compile): $cmd" >&9
  cat ${dlname}.d >&9
  eval ${cmd} >&9 2>&9
  rc=$?
  if [ $rc -lt 0 ]; then
    _exitmkconfig $rc
  fi
  puts "##      _link compile: $rc" >&9

  cmd="${_MKCONFIG_DIR}/mkc.sh -d `pwd` -complink -e -c ${DC} -o ${dlname}.exe \
      -- ${DFLAGS} ${dlname}${OBJ_EXT} ${LDFLAGS} ${LIBS} "
  _dlotherlibs=$otherlibs
  if [ "${_dlotherlibs}" != "" ]; then
    cmd="${cmd} ${_dlotherlibs} "
  fi
  puts "##  _link test (link): $cmd" >&9
  cat ${dlname}.d >&9
  eval $cmd >&9 2>&9
  rc=$?
  if [ $rc -lt 0 ]; then
    _exitmkconfig $rc
  fi
  puts "##      _link link: $rc" >&9
  if [ $rc -eq 0 ]; then
    if [ ! -x "${dlname}.exe" ]; then  # not executable
      rc=1
    fi
  fi
  return $rc
}

_d_chk_compile () {
  dfname=$1
  code=$2
  inc=$3

  tdfile=${dfname}.d
  # $dfname should be unique
  exec 4>>${tdfile}
  _d_print_imports $inc >&4
  puts "${code}" | sed 's/_dollar_/$/g' >&4
  exec 4>&-

  cmd="${_MKCONFIG_DIR}/mkc.sh -d `pwd` -complink -e -c ${DC} \
      -o ${dfname}${OBJ_EXT} -- ${DFLAGS} ${tdfile} "
  puts "##  compile test: $cmd" >&9
  cat ${dfname}.d >&9
  eval ${cmd} >&9 2>&9
  rc=$?
  puts "##  compile test: $rc" >&9
  return $rc
}

do_d_check_compile () {
  ddfname=$1
  code=$2
  inc=$3

  _d_chk_compile ${ddfname} "${code}" $inc
  rc=$?
  try=0
  if [ $rc -eq 0 ]; then
    try=1
  fi
  printyesno $ddfname $try
  setdata ${_MKCONFIG_PREFIX} ${ddfname} ${try}
}

