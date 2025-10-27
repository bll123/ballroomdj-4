#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "Usage: $0 <shortcut-name> <install-path> <working-dir> <profile-num>"
  exit 1
fi

DVERS=3

# the gnome-desktop's dock uses the StartupWMClass entry in the
# .desktop files to figure out the icon for the process, and gets
# the icon from the $HOME/.local/share/icons/hicolor/ hierarchy.
# A .desktop file for each of the different processes must be installed.
# iconlist: key: executable-suffix data: icon-name
declare -A ICONLIST
ICONLIST=( altinst inst \
    bpmcounter bpm \
    configui config \
    helperui help \
    manageui manage \
    marquee marquee \
    playerui player \
    subt subt \
    )

# the main .desktop for the installation or alternate installation
# should be installed in both $HOME/Desktop
# and $HOME/.local/share/applications.
function installmainsc {
  for idir in "$desktop" "$HOME/.local/share/applications"; do
    if [[ ! -d $idir ]]; then
      continue
    fi

    rc=1
    fpath="$idir/${fullscname}.desktop"
    if [[ -f ${fpath} ]]; then
      grep -l "^# version ${DVERS}" "${fpath}" >/dev/null 2>&1
      rc=$?
    fi

    if [[ $rc -ne 0 ]]; then
      cp -f "${tgtpath}/install/bdj4.desktop" "${fpath}"
      sed -i -e "s,#INSTALLPATH#,${tgtpath}," \
          -e "s,#APPNAME#,${scname}," \
          -e "s,#WORKDIR#,${workdir}," \
          -e "s,#PROFILE#,${profargs}," \
          "${fpath}"
    fi
  done
}

# the multitude of .desktop files for gnome-desktop only need
# to be installed once, and can all use the same BDJ4 prefix.
# these use a minimal .desktop file with the icon name and wmclass.
# NoDisplay=true is set.
function installappsc {
  tscname=BDJ4

  idir="$HOME/.local/share/applications"
  if [[ ! -d $idir ]]; then
    continue
  fi

  for win in ${!ICONLIST[@]}; do
    icon=${ICONLIST[$win]}
    fpath="$idir/${tscname}_${win}.desktop"

    rc=1
    if [[ -f ${fpath} ]]; then
      grep -l '^# version 3' "${fpath}" >/dev/null 2>&1
      rc=$?
    fi

    if [[ $rc -ne 0 ]]; then
      cp -f "${tgtpath}/install/bdj4win.desktop" "${fpath}"
      sed -i -e "s,#ICONNAME#,bdj4_icon_${icon}," \
          -e "s,#WMCLASS#,bdj4${win}," \
          -e "s,#APPNAME#,${scname}," \
          "${fpath}"
    fi
  done
}

scname=$1
tgtpath=$2
workdir=$3
profile=$4

# make sure the created .desktop file has a prefix of BDJ4
# this make them easy to remove.
fullscname="${scname}"
case "${scname}" in
  BDJ4*)
    ;;
  *)
    fullscname="BDJ4${scname}"
    ;;
esac

if [[ ! -d "$tgtpath" || \
    ! -d "$tgtpath/install" || \
    ! -f "$tgtpath/install/bdj4.desktop" ]]; then
  echo "Could not locate $tgtpath"
  exit 1
fi

profargs=""
if [[ $profile != "" && $profile -gt 0 ]]; then
  profargs=" --profile $profile"
fi

desktop=$(xdg-user-dir DESKTOP)
for idir in "$desktop" "$HOME/.local/share/applications"; do
  if [[ ! -d ${idir} ]]; then
    # KDE did not have a $HOME/.local/share/applications folder
    mkdir -p "${idir}"
  fi
  if [[ ${scname} == BDJ4 ]]; then
    # remove old shortcuts that have a lower case name
    tname=bdj4
    fpath="$idir/${tname}.desktop"
    if [[ -f ${fpath} ]]; then
      rm -f "${fpath}"
    fi
  fi
done

installmainsc
installappsc

exit 0
