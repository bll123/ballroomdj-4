#!/bin/bash

systype=$(uname -s)

manfn=install/manifest.txt
mantmp=install/manifest.n

findlist="LICENSE.txt README.txt VERSION.txt \
    bin conv img install licenses locale templates"

case ${systype} in
  Darwin)
    findlist="$findlist plocal/bin plocal/share/themes/macOS* plocal/share/icons"
    ;;
  MSYS*|MINGW*)
    findlist="$findlist plocal/bin plocal/share/themes/Wind* plocal/share/icons"
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
