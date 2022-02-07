#!/bin/bash

systype=$(uname -s)

manfn=install/manifest.txt
mantmp=install/manifest.n

findlist="LICENSE.txt README.txt VERSION.txt \
    bin conv img install licenses locale templates"
case ${systype} in
  MSYS*|MINGW*)
    findlist="$findlist plocal/bin"
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
    cat ${manfn} |
      sed -e 's,\.so$,.dylib,' \
      > ${mantmp}
    mv -f ${mantmp} ${manfn}
    ;;
  MSYS*|MINGW**)
    cat ${manfn} |
      sed -e 's,bin/\([^\.]*\),bin/\1.exe,' \
          -e 's,\.so$,.dll,' \
          -e 's,\.sh$,.bat,' \
      > ${mantmp}
    mv -f ${mantmp} ${manfn}
    ;;
esac
