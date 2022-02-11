#!/bin/bash

systype=$(uname -s)

manfn=install/manifest.txt
mantmp=install/manifest.n

findlist="LICENSE.txt README.txt VERSION.txt \
    bin conv img install licenses locale templates"

case ${systype} in
  Darwin)
    findlist="$findlist plocal/share/themes/macOS*"
    ;;
  MSYS*|MINGW*)
    findlist="$findlist plocal/bin plocal/share/themes/Wind*"
    findlist="$findlist plocal/share/icons plocal/lib/gdk-pixbuf-2.0"
    findlist="$findlist plocal/share/glib-2.0/schemas plocal/etc/gtk-3.0"
    findlist="$findlist plocal/lib/girepository-1.0 plocal/etc/fonts"
    ;;
esac

find ${findlist} -type f -print \
      | sort > ${manfn}
sed -e '/check_all/ d' \
    -e '/bdj4cli/ d' \
    -e '\,img/README, d' \
    -e '\,img/mkicon.sh, d' \
    -e '/bdj4se/ d' \
    -e '/chkprocess/ d' \
    -e '/checkmk/ d' \
    -e '/curl-config/ d' \
    -e '/libcheck.*dll/ d' \
    -e '/ocspcheck.exe/ d' \
    -e '/openssl.exe/ d' \
    ${manfn} > ${mantmp}
    mv -f ${mantmp} ${manfn}

case $systype in
  Darwin)
    ;;
  MSYS*|MINGW**)
    # a) remove .sh files from manifest
    cat ${manfn} |
      sed -e '/\.sh$/ d' \
      > ${mantmp}
    mv -f ${mantmp} ${manfn}
    ;;
esac
