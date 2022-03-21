#!/bin/bash

spath=$(dirname $0)
RESTFILE="$spath/.ballroomdj-ss-restore"

restoreflag=F
if [[ -f $RESTFILE ]]; then
  chmod a+rx $RESTFILE
  . $RESTFILE
  restoreflag=T
else
  > $RESTFILE
fi

function do_gsettings () {
  if [ ! -f /usr/bin/gsettings ]; then
    return
  fi
  schema=$1
  setting=$2
  shift; shift
  for skey in $*; do
    val=$(gsettings get $schema $skey 2>/dev/null)
    if [[ $? -eq 0 ]]; then
      if [[ $restoreflag == F ]]; then
        echo "gsettings set $schema $skey $val" >> $RESTFILE
      fi
      gsettings set $schema $skey $setting
    fi
  done
}

function do_xfcesettings () {
  if [ ! -f /usr/bin/xfconf-query ]; then
    return 2
  fi
  rc=1
  schema=$1
  setting=$2
  shift; shift
  for skey in $*; do
    val=$(xfconf-query -c $schema -p /$schema/$skey 2>/dev/null)
    if [[ $? -eq 0 ]]; then
      rc=0
      if [[ $restoreflag == F ]]; then
        echo "xfconf-query -c $schema -p /$schema/$skey -s $val" >> $RESTFILE
      fi
      xfconf-query -c $schema -p /$schema/$skey -s $setting
    fi
  done
  return $rc
}

# X power management
xset -dpms
xset s off
setterm -powersave off -blank 0 2>/dev/null

# gnome power management/screensaver
schema=org.gnome.settings-daemon.plugins.power
do_gsettings $schema false \
    active
schema=org.gnome.desktop.screensaver
do_gsettings $schema false \
    idle-activation-enabled

# mate power management/screensaver
schema=org.mate.power-manager
do_gsettings $schema 0 \
    sleep-computer-ac \
    sleep-display-ac \
    sleep-computer-battery \
    sleep-display-battery
do_gsettings $schema false \
    backlight-battery-reduce

schema=org.mate.screensaver
do_gsettings $schema false \
    idle-activation-enabled

# xfce power settings
# presentation-mode may not exist in settings the first time, force it.
if [ -f /usr/bin/xfconf-query ]; then
  xfconf-query -c xfce4-power-manager -p /xfce4-power-manager/presentation-mode -s false
fi
do_xfcesettings xfce4-power-manager true \
    presentation-mode
rc=$?
if [ $rc -eq 0 ]; then
  do_xfcesettings xfce4-power-manager 0 \
      dpms-on-ac-off dpms-on-ac-sleep dpms-on-battery-off \
      dpms-on-battery-sleep inactivity-on-ac inactivity-on-battery \
      blank-on-ac blank-on-battery brightness-on-ac brightness-on-battery
fi

if [[ -f $RESTFILE && ! -f $RESTFILE.orig ]]; then
  cp -f $RESTFILE $RESTFILE.orig
fi
