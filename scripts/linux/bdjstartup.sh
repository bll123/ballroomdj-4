#!/bin/bash

# old path / cleanup
cdir="$HOME/.config"
rm -f "${cdir}/BDJ4/bdj4-ss-restore"
rm -f "${cdir}/BDJ4/bdj4-ss-restore.orig"

cdir="${XDG_CACHE_DIR:-$HOME/.cache}"
spath="${cdir}/BDJ4"
test -d "$spath" || mkdir -p "$spath"
RESTFILE="$spath/bdj4-ss-restore"

restoreflag=F
if [[ -f $RESTFILE ]]; then
  chmod a+rx $RESTFILE
  . $RESTFILE
  restoreflag=T
else
  > $RESTFILE
fi

function do_gsettings () {
  if [[ ! -f /usr/bin/gsettings ]]; then
    return 2
  fi
  rc=1
  schema=$1
  setting=$2
  shift; shift
  for skey in $*; do
    val=$(gsettings get $schema $skey 2>/dev/null)
    if [[ $? -eq 0 ]]; then
      rc=0
      if [[ $restoreflag == F ]]; then
        echo "gsettings set $schema $skey $val" >> $RESTFILE
      fi
      gsettings set $schema $skey $setting
    fi
  done

  return $rc
}

function do_xfcesettings () {
  if [[ ! -f /usr/bin/xfconf-query ]]; then
    return 2
  fi
  rc=1

  process=$1
  schema=$2
  setting=$3
  shift; shift; shift

  for skey in $*; do
    case $skey in
      /*)
        tskey=$skey
        ;;
      *)
        tskey=/$schema/$skey
        ;;
    esac

    trc=1
    type=""
    case $process in
      bool)
        trc=0
        val=false
        type="-t $process"
        ;;
      check)
        val=$(xfconf-query -c $schema -p $skey 2>/dev/null)
        trc=$?
        ;;
    esac

    if [[ $trc -eq 0 ]]; then
      rc=0
      if [[ $restoreflag == F ]]; then
        echo "xfconf-query -c $schema -p $tskey -s $val" >> $RESTFILE
      fi
      xfconf-query --create -c $schema -p $tskey $type -s $setting
    fi
  done
  return $rc
}

if [[ $XDG_SESSION_TYPE == x11 && -f /usr/bin/xset ]]; then
  # X power management
  xset -dpms
  xset s off
  setterm -powersave off -blank 0 2>/dev/null
fi

case $XDG_SESSION_DESKTOP in
  KDE)
    # there is no way to set presentation mode via a script
    # because kde wants the process to stay alive
    # 4.17.3.1 : startutil.c : call dbus to set presentation mode for KDE.
    ;;
  xfce|XFCE)
    # xfce power settings
    do_xfcesettings bool xfce4-power-manager true \
        presentation-mode
    rc=$?
    if [[ $rc -eq 0 ]]; then
      do_xfcesettings check xfce4-notifyd true \
          /do-not-disturb
    fi
    if [[ $rc -eq 1 ]]; then
      # do not need to do any of this if presentation mode is available
      do_xfcesettings check xfce4-power-manager 0 \
          dpms-on-ac-off dpms-on-ac-sleep dpms-on-battery-off \
          dpms-on-battery-sleep inactivity-on-ac inactivity-on-battery \
          blank-on-ac blank-on-battery brightness-on-ac brightness-on-battery
    fi
    ;;
  mate)
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

    # mate notifications
    schema=org.mate.caja.preferences
    do_gsettings $schema false \
        show-notifications
    ;;
  *)
    # gnome
    # cinnamon
    schema=org.gnome.settings-daemon.plugins.power
    do_gsettings $schema false \
        active

    schema=org.gnome.desktop.screensaver
    do_gsettings $schema false \
        idle-activation-enabled

    # gnome notifications
    schema=org.gnome.desktop.notifications
    do_gsettings $schema false \
        show-banners
    ;;
esac

# create a backup; this is mostly useful for development purposes
if [[ ! -f $RESTFILE.orig ]]; then
  cp -pf $RESTFILE $RESTFILE.orig
fi
