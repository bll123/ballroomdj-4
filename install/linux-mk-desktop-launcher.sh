#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "Usage: $0 <shortcut-name> <install-path> <working-dir> <profile-num>"
  exit 1
fi

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

if [[ ! -d "$tgtpath" ]]; then
  echo "Could not locate $tgtpath"
  exit 1
fi

profargs=""
if [[ $profile != "" && $profile -gt 0 ]]; then
  profargs=" --profile $profile"
fi

desktop=$(xdg-user-dir DESKTOP)
for idir in "$desktop" "$HOME/.local/share/applications"; do
  if [[ ${scname} == BDJ4 ]]; then
    # remove old shortcuts that have a lower case name
    tname=bdj4
    fpath="$idir/${tname}.desktop"
    if [[ -f ${fpath} ]]; then
      rm -f "${fpath}"
    fi
  fi
  fpath="$idir/${fullscname}.desktop"
  if [[ -d $idir ]]; then
      cp -f "${tgtpath}/install/bdj4.desktop" "${fpath}"
      sed -i -e "s,#INSTALLPATH#,${tgtpath},g" \
          -e "s,#APPNAME#,${scname},g" \
          -e "s,#WORKDIR#,${workdir},g" \
          -e "s,#PROFILE#,${profargs},g" \
          "${fpath}"
  fi
done

exit 0
