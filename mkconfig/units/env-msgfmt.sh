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
# optional unit: cflags

check_cmd_msgfmt () {
  name="$1"

  name=XMSGFMT

  printlabel $name "command: locate msgfmt"
  checkcache_val ${_MKCONFIG_PREFIX} $name
  if [ $? -eq 0 ]; then return; fi

  locatecmd xmsgfmt msgfmt
  locatecmd xgmsgfmt gmsgfmt

  mfmt="${xmsgfmt}"
  if [ "$_MKCONFIG_USING_GCC" = "Y" ]
  then
      mfmt="${xgmsgfmt:-${xmsgfmt}}"
      if [ -x "${xccpath}/msgfmt" ]
      then
          mfmt="${xccpath}/msgfmt"
      fi
      if [ -x "${xccpath}/gmsgfmt" ]
      then
          mfmt="${xccpath}/gmsgfmt"
      fi
  fi

  printyesno_val XMSGFMT $xmsgfmt
  setdata ${_MKCONFIG_PREFIX} "XMSGFMT" "${xmsgfmt}"
}
