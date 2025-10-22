#!/bin/bash

cdir="${XDG_CACHE_DIR:-$HOME/.cache}"
spath="${cdir}/BDJ4"
test -d "$spath" || mkdir -p "$spath"
RESTFILE="$spath/bdj4-ss-restore"

if [[ $XDG_SESSION_TYPE == x11 && -f /usr/bin/xset ]]; then
  # turn screen savers/power management back on
  xset +dpms
  xset s on
  setterm -powersave on -blank 5 2>/dev/null
fi

if [[ -f $RESTFILE ]]; then
  chmod a+rx $RESTFILE
  . $RESTFILE
  rm -f $RESTFILE
fi
