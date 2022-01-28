#!/bin/sh
#
# Copyright 2001-2018 Brad Lanam, Walnut Creek, California, USA
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

check_extension () {
  type=$2

  name="${type}ext"
  eval check_${name}
}

check_objext () {
  name=OBJ_EXT
  printlabel $name "extension: object"

  TMPF=objext

  CC=${CC:-cc}

  > $TMPF.c puts '
#include <stdio.h>
int main ()
{
  printf ("hello\n");
  return 0;
}
'

  ${CC} -c $TMPF.c > /dev/null 2>&1 # don't care about warnings...
  OBJ_EXT=".o"
  if [ -f "$TMPF.obj" ]; then
     puts "object extension is .obj" >&9
     OBJ_EXT=".obj"
  else
     puts "object extension is .o" >&9
  fi

  printyesno_val $name "${OBJ_EXT}"
  setdata ${_MKCONFIG_PREFIX} $name "${OBJ_EXT}"
}

check_exeext () {
  name=EXE_EXT
  printlabel $name "extension: executable"

  TMPF=exeext

  CC=${CC:-cc}

  > $TMPF.c puts '
#include <stdio.h>
int main ()
{
  printf ("hello\n");
  return 0;
}
'

  ${CC} -o $TMPF $TMPF.c > /dev/null 2>&1 # don't care about warnings
  EXE_EXT=""
  if [ -f "$TMPF.exe" ]
  then
     puts "executable extension is .exe" >&9
     EXE_EXT=".exe"
  else
     puts "executable extension is none" >&9
  fi

  printyesno_val $name "${EXE_EXT}"
  setdata ${_MKCONFIG_PREFIX} $name "${EXE_EXT}"
}

check_shlibext () {
  name=SHLIB_EXT
  printlabel $name "extension: shared library"

  SHLIB_EXT=".so"
  case ${_MKCONFIG_SYSTYPE} in
    HP-UX)
      SHLIB_EXT=".sl"
      ;;
    AIX)
      SHLIB_EXT=".a"
      ;;
    Darwin)
      SHLIB_EXT=".dylib"
      ;;
    CYGWIN*|MSYS*|MINGW*)
      SHLIB_EXT=".dll"
      ;;
  esac

  printyesno_val $name "$SHLIB_EXT"
  setdata ${_MKCONFIG_PREFIX} $name "$SHLIB_EXT"
}


