#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "Usage: $0 <shortcut-name> <install-path> <working-dir> <profile-num>"
  exit 1
fi

scname=$1
tgtpath=$2
workdir=$3
profile=$4

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
  fpath="$idir/${scname}.desktop"
  if [[ -d $idir ]]; then
    if [[ ! -f $fpath ]]; then
      cp -f install/bdj4.desktop "${fpath}"
      sed -i -e "s,#INSTALLPATH#,${tgtpath},g" \
          -e "s,#APPNAME#,${scname},g" \
          -e "s,#WORKDIR#,${workdir},g" \
          -e "s,#PROFILE#,${profargs},g" \
          "${fpath}"
    fi
  fi
done

exit 0
