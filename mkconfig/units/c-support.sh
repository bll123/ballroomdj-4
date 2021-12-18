#!/bin/sh
#
# Copyright 2010-2018 Brad Lanam Walnut Creek CA USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#

CPPCOUNTER=1

  postcc="
/* some gcc's (cygwin) redefine __restrict again */
#if defined (__restrict)
# undef __restrict
#endif
#define __restrict
"

_c_print_headers () {
  incheaders=$1
  cppchk=$2

  if [ "$incheaders" = "none" ]; then
    return
  fi

  out="${PH_PREFIX}${incheaders}"

  if [ -f $out ]; then
    cat $out
    return
  fi

  if [ "$PH_STD" = "T" -a "$incheaders" = "std" ]; then
    _c_print_hdrs std $cppchk > $out
    cat $out
    return
  fi

  if [ "$PH_ALL" = "T" -a "$incheaders" = "all" ]; then
    _c_print_hdrs all $cppchk > $out
    cat $out
    return
  fi

  # until PH_STD/PH_ALL becomes true, just do normal processing.
  _c_print_hdrs $incheaders $cppchk
}

_c_print_hdrs () {
  incheaders=$1
  cppchk=$2

  if [ "${incheaders}" = "all" -o "${incheaders}" = "std" ]; then
    for tnm in '_hdr_stdio' '_hdr_stdlib' '_sys_types' '_sys_param'; do
      getdata tval ${_MKCONFIG_PREFIX} ${tnm}
      if [ "${tval}" != "0" -a "${tval}" != "" ]; then
        puts "#include <${tval}>"
        # for cygwin/gcc
        if [ "$cppchk" = T -a $tnm = _hdr_stdio ]; then
          puts "${postcc}"
        fi
      fi
    done
  fi

  if [ "${incheaders}" = "all" -a -f "$VARSFILE" ]; then
    # save stdin in fd 6; open stdin
    exec 6<&0 < ${VARSFILE}
    while read cfgvar; do
      getdata hdval ${_MKCONFIG_PREFIX} ${cfgvar}
      case ${cfgvar} in
        _hdr_stdio|_hdr_stdlib|_sys_types|_sys_param)
          ;;
        _hdr_linux_quota)
          if [ "${hdval}" != "0" ]; then
            getdata iqval ${_MKCONFIG_PREFIX} '_inc_conflict__sys_quota__hdr_linux_quota'
            if [ "${iqval}" = "1" ]; then
              puts "#include <${hdval}>"
            fi
          fi
          ;;
        _sys_time)
          if [ "${hdval}" != "0" ]; then
            getdata itval ${_MKCONFIG_PREFIX} '_inc_conflict__hdr_time__sys_time'
            if [ "${itval}" = "1" ]; then
              puts "#include <${hdval}>"
            fi
          fi
          ;;
        _hdr_*|_sys_*)
          if [ "${hdval}" != "0" -a "${hdval}" != "" ]; then
            puts "#include <${hdval}>"
          fi
          ;;
      esac
    done
    # set std to saved fd 6; close 6
    exec <&6 6<&-
  fi
}

_c_chk_run () {
  crname=$1
  code=$2
  inc=$3

  _c_chk_link_libs ${crname} "${code}" $inc
  rc=$?
  puts "##  run test: link: $rc" >&9
  rval=0
  if [ $rc -eq 0 ]; then
    rval=`./${crname}.exe`
    rc=$?
    puts "##  run test: run: $rc retval:$rval" >&9
    if [ $rc -lt 0 ]; then
      _exitmkconfig $rc
    fi
  fi
  _retval=$rval
  return $rc
}

_c_chk_link_libs () {
  cllname=$1
  code=$2
  inc=$3
  shift;shift;shift

  ocounter=0
  clotherlibs="'$otherlibs'"
  dosubst clotherlibs ',' "' '"
  if [ "${clotherlibs}" != "" ]; then
    eval "set -- $clotherlibs"
    ocount=$#
  else
    ocount=0
  fi

  tcfile=${cllname}.c
  >${tcfile}
  # $cllname should be unique
  exec 4>>${tcfile}
  puts "${precc}" >&4
  _c_print_headers $inc >&4
  puts "${code}" | sed 's/_dollar_/$/g' >&4
  exec 4>&-

  dlibs=""
  otherlibs=""
  _c_chk_link $cllname
  rc=$?
  puts "##      link test (none): $rc" >&9
  if [ $rc -ne 0 ]; then
    while test $ocounter -lt $ocount; do
      domath ocounter "$ocounter + 1"
      eval "set -- $clotherlibs"
      cmd="olibs=\$${ocounter}"
      eval $cmd
      dlibs=${olibs}
      otherlibs=${olibs}
      _c_chk_link $cllname
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

_c_chk_cpp () {
  cppname=$1
  code="$2"
  inc=$3

  tcppfile=${cppname}.c
  tcppout=${cppname}.out
  tcppreuse=F
  if [ "$code" = "" -a $inc = all ]; then
    tcppreuse=T
    tcppfile=chkcpp_${CPPCOUNTER}.c
    tcppout=chkcpp_${CPPCOUNTER}.out

    if [ -f $tcppfile -a -f $tcppout ]; then
      test -f ${cppname}.out && rm -f ${cppname}.out
      ln -s ${tcppout} ${cppname}.out
      puts "##  _cpp test: reusing $tcppout" >&9
      rc=0
      return $rc
    fi
  fi
  # $cppname should be unique
  exec 4>>${tcppfile}
  puts "${precc}" >&4
  _c_print_headers $inc T >&4
  puts "${code}" | sed 's/_dollar_/$/g' >&4
  exec 4>&-

  setcflags
  cmd="${CC} ${CFLAGS} -E ${tcppfile} > ${tcppout} "
  puts "##  _cpp test: $cmd" >&9
  cat ${tcppfile} >&9
  eval $cmd >&9 2>&9
  rc=$?
  if [ $rc -lt 0 ]; then
    _exitmkconfig $rc
  fi

  if [ $tcppreuse = T ]; then
    test -f ${cppname}.out && rm -f ${cppname}.out
    ln -s ${tcppout} ${cppname}.out
  fi
  puts "##      _cpp test: $rc" >&9
  return $rc
}

_c_chk_link () {
  clname=$1

  setcflags
  setldflags
  setlibs
  cmd="${CC} ${CFLAGS} -o ${clname}.exe ${clname}.c "
  cmd="${cmd} ${LDFLAGS} ${LIBS} "
  _clotherlibs=$otherlibs
  if [ "${_clotherlibs}" != "" ]; then
    cmd="${cmd} ${_clotherlibs} "
  fi
  puts "##  _link test: $cmd" >&9
  cat ${clname}.c >&9
  eval $cmd >&9 2>&9
  rc=$?
  if [ $rc -lt 0 ]; then
    _exitmkconfig $rc
  fi
  puts "##      _link test: $rc" >&9
  if [ $rc -eq 0 ]; then
    if [ ! -x "${clname}.exe" ]; then  # not executable
      rc=1
    fi
  fi
  return $rc
}


_c_chk_compile () {
  ccname=$1
  code=$2
  inc=$3

  tcfile=${ccname}.c
  >${tcfile}
  # $ccname should be unique
  exec 4>>${tcfile}
  puts "${precc}" >&4
  _c_print_headers $inc >&4
  puts "${code}" | sed 's/_dollar_/$/g' >&4
  exec 4>&-

  setcflags
  cmd="${CC} ${CFLAGS} -c ${tcfile}"
  puts "##  compile test: $cmd" >&9
  cat ${ccname}.c >&9
  eval ${cmd} >&9 2>&9
  rc=$?
  puts "##  compile test: $rc" >&9
  return $rc
}


do_c_check_compile () {
  dccname=$1
  code=$2
  inc=$3

  _c_chk_compile ${dccname} "${code}" $inc
  rc=$?
  try=0
  if [ $rc -eq 0 ]; then
    try=1
  fi
  printyesno $dccname $try
  setdata ${_MKCONFIG_PREFIX} ${dccname} ${try}
}

