#!/bin/sh
#
# Copyright 2010-2018 Brad Lanam, Walnut Creek, California USA
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
require_unit env-systype

check_dc () {
  _read_option DC gdc
  DC=${DC:-${oval}}

  printlabel DC "D compiler"

  puts "dc:${DC}" >&9

  case ${DC} in
    *ldc*|*ldmd*)
      setdata ${_MKCONFIG_PREFIX} DC_OPT "-O3"
      setdata ${_MKCONFIG_PREFIX} DC_OF "-of"
      setdata ${_MKCONFIG_PREFIX} DC_RELEASE "-release"
      setdata ${_MKCONFIG_PREFIX} DC_INLINE ""
      setdata ${_MKCONFIG_PREFIX} DC_UNITTEST "-unittest"
      setdata ${_MKCONFIG_PREFIX} DC_DEBUG "-d-debug"
      setdata ${_MKCONFIG_PREFIX} DC_VERSION "-d-version"
      setdata ${_MKCONFIG_PREFIX} DC_INTFC "-d-intfc"
      setdata ${_MKCONFIG_PREFIX} DC_DEPRECATED "-d-deprecated"
      setdata ${_MKCONFIG_PREFIX} DC_COV ""
      setdata ${_MKCONFIG_PREFIX} DC_LINK "-L"
      setdata ${_MKCONFIG_PREFIX} _MKCONFIG_USING_GDC "N"
      ;;
    *dmd*)
      setdata ${_MKCONFIG_PREFIX} DC_OPT "-O"
      setdata ${_MKCONFIG_PREFIX} DC_OF "-of"
      setdata ${_MKCONFIG_PREFIX} DC_RELEASE "-release"
      setdata ${_MKCONFIG_PREFIX} DC_INLINE "-inline"
      setdata ${_MKCONFIG_PREFIX} DC_UNITTEST "-unittest"
      setdata ${_MKCONFIG_PREFIX} DC_DEBUG "-debug"
      setdata ${_MKCONFIG_PREFIX} DC_VERSION "-version"
      setdata ${_MKCONFIG_PREFIX} DC_INTFC "-H"
      setdata ${_MKCONFIG_PREFIX} DC_DEPRECATED "-d"
      setdata ${_MKCONFIG_PREFIX} DC_COV "-cov"
      setdata ${_MKCONFIG_PREFIX} DC_LINK "-L"
      setdata ${_MKCONFIG_PREFIX} _MKCONFIG_USING_GDC "N"
      ;;
    *gdc*)
      setdata ${_MKCONFIG_PREFIX} DC_OPT "-O2"
      setdata ${_MKCONFIG_PREFIX} DC_OF "-o"
      setdata ${_MKCONFIG_PREFIX} DC_RELEASE "-frelease"
      setdata ${_MKCONFIG_PREFIX} DC_INLINE "--inline"
      setdata ${_MKCONFIG_PREFIX} DC_UNITTEST "-funittest"
      setdata ${_MKCONFIG_PREFIX} DC_DEBUG "--debug"
      setdata ${_MKCONFIG_PREFIX} DC_VERSION "-fversion"
      setdata ${_MKCONFIG_PREFIX} DC_INTFC "-fintfc"
      setdata ${_MKCONFIG_PREFIX} DC_DEPRECATED "-fdeprecated"
      setdata ${_MKCONFIG_PREFIX} DC_COV "--cov"
      setdata ${_MKCONFIG_PREFIX} DC_LIBS "-lgcov"
      setdata ${_MKCONFIG_PREFIX} DC_LINK ""
      setdata ${_MKCONFIG_PREFIX} _MKCONFIG_USING_GDC "Y"
      ;;
  esac

  > tv.d puts '
int main (char[][] args) {
  version (D_Version2) { return 2; }
  version (D_Version3) { return 3; }
  version (D_Version4) { return 4; }
  version (D_Version5) { return 5; }
  version (D_Version6) { return 6; }
  return 1; }
'

  dver=
  grc=1
  ${DC} ${DC_OF}tv.exe tv.d
  rc=$?
  if [ $rc -eq 0 ]; then
    if [ ! -x ./tv.exe ]; then
      rc=127
    else
      ./tv.exe
      rc=$?
    fi
    case $rc in
      [1-9])
        dver=$rc
        grc=0
        ;;
      *)
        ;;
    esac
  fi
  if [ $grc -ne 0 ]; then
    puts "## Failure to determine D version"
    exit 1
  fi
  setdata ${_MKCONFIG_PREFIX} DVERSION $dver

  printyesno_val DC "${DC}" "v${dver}"
  setdata ${_MKCONFIG_PREFIX} DC "${DC}"
}

check_using_gdc () {
  usinggdc="N"

  printlabel _MKCONFIG_USING_GDC "Using gdc"

  # check for gdc...
  ${DC} -v 2>&1 | grep 'gdc version' > /dev/null 2>&1
  rc=$?
  if [ $rc -eq 0 ]
  then
      puts "found gdc" >&9
      usinggdc="Y"
  fi

  case ${DC} in
      *gdc*)
          usinggdc="Y"
          ;;
  esac

  printyesno_val _MKCONFIG_USING_GDC "${usinggdc}"
  setdata ${_MKCONFIG_PREFIX} _MKCONFIG_USING_GDC "${usinggdc}"
}

check_dflags () {
  _read_option DFLAGS ""
  dflags="${DFLAGS:-${oval}}"
  _read_option DINCLUDES ""
  dincludes="${DINCLUDES:-${oval}}"

  printlabel DFLAGS "D flags"

  gdcflags=""

  if [ "${_MKCONFIG_USING_GDC}" = "Y" ]
  then
      puts "set gdc flags" >&9
      gdcflags=""
  fi

  TDC=${DC}
  if [ "${_MKCONFIG_USING_GDC}" = "Y" ]
  then
    TDC=gdc
  fi

  dflags="$gdcflags $dflags"

  puts "dflags:${dflags}" >&9

  printyesno_val DFLAGS "$dflags"
  setdata ${_MKCONFIG_PREFIX} DFLAGS "$dflags"
}
