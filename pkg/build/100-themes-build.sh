#!/bin/bash

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
      (cd ${themedir};unzip -q "$ffn")
    done
    for tn in ../plocal/share/themes/*; do
      nn="$(echo $tn | sed 's,-dark$,,')"
      nn="$(echo $nn | sed 's,-[0-9.]*$,,')"
      mv "$tn" "$nn"
    done
  fi
fi

