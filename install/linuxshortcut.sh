#!/bin/bash

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <install-path>"
  exit 1
fi

tgtpath=$1

if [[ ! -d "$tgtpath" ]]; then
  echo "Could not locate $tgtpath"
  exit 1
fi

desktop=$(xdg-user-dir DESKTOP)
for idir in "$desktop" "$HOME/.local/share/applications"; do
  if [ -d "$idir" ]; then
    cp -f install/bdj4.desktop $idir
    sed -i -e "s,#INSTALLPATH#,${tgtpath},g" \
        $idir/bdj4.desktop
  fi
done

exit 0
