#!/bin/bash

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 <shortcut-name> <install-path> <working-dir>"
  exit 1
fi

scname=$1
tgtpath=$2
workdir=$3

if [[ ! -d "$tgtpath" ]]; then
  echo "Could not locate $tgtpath"
  exit 1
fi

desktop=$(xdg-user-dir DESKTOP)
for idir in "$desktop" "$HOME/.local/share/applications"; do
  if [ -d "$idir" ]; then
    cp -f install/bdj4.desktop "$idir/${scname}.desktop"
    sed -i -e "s,#INSTALLPATH#,${tgtpath},g" \
        -e "s,#APPNAME#,${scname},g" \
        -e "s,#WORKDIR#,${workdir},g" \
        "$idir/${scname}.desktop"
  fi
done

exit 0
