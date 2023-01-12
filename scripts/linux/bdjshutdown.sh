#!/bin/bash

spath=$HOME/.config/BDJ4
test -d "$spath" || mkdir -p "$spath"
RESTFILE="$spath/.ballroomdj-ss-restore"

# turn screen savers/power management back on
xset +dpms
xset s on
setterm -powersave on -blank 5 2>/dev/null

if [[ -f $RESTFILE ]]; then
  chmod a+rx $RESTFILE
  . $RESTFILE
  rm -f $RESTFILE
fi
