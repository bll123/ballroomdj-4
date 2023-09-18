#!/bin/bash

case ${systype} in
  Linux)
    ;;
  Darwin)
    ;;
  MINGW64*)
    ;;
esac

# fpcalc is pre-built
# just install it.

if [[ $pkgname == "" || $pkgname = "acrcloud" ]]; then
  cd $cwd
  if [ $? -eq 0 ]; then
    echo ""
    echo "## install ${pkgname}"
    cp -f acrcloud-${tag}${archtag}${esuffix} $INSTLOC/bin/acrcloud${esuffix}
  fi
fi

