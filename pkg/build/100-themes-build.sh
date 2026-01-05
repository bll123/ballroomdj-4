#!/bin/bash
#
# Copyright 2023-2026 Brad Lanam Pleasant Hill CA
#

case ${systype} in
  Linux)
    exit 0
    ;;
  Darwin)
    ;;
  MINGW64*)
    ;;
esac

if [[ $pkgname == "" || $pkgname = "themes" ]]; then
  cd $cwd
  echo ""
  echo "## install themes"
  themedir=../plocal/share/themes
  test -d "${themedir}" || mkdir -p ${themedir}
  if [[ ${systype} == Darwin ]]; then
    for fn in bundles/Mojave*; do
      ffn="${cwd}/$fn"
      (cd ${themedir};tar xf "${ffn}")
    done
  fi
  if [[ ${platform} == windows ]]; then
    for fn in bundles/Windows-*.zip; do
      ffn="${cwd}/$fn"
      (cd ${themedir};unzip -q -o "$ffn")
    done
    for tn in ../plocal/share/themes/*; do
      nn="$(echo $tn | sed 's,-dark$,,')"
      nn="$(echo $nn | sed 's,-[0-9.]*$,,')"
      if [[ $tn != $nn ]]; then
        if [[ -d $nn ]]; then
          rm -rf "$nn"
        fi
        mv "$tn" "$nn"
      fi
    done
    for tn in ../plocal/share/themes/*; do
      for d in cinnamon gnome-shell gtk-2.0 metacity-1 plank openbox-3 unity xfce-notify-4.0 xfwm4; do
        if [[ -d "$tn/$d" ]]; then
          rm -rf "$tn/$d"
        fi
      done
    done
  fi
fi

