#!/bin/bash

find LICENSE.txt README.txt VERSION.txt \
      bin conv img install locale plocal/bin templates \
      -type f -print |
    grep -v 'check_all' |
    grep -v 'clicomm' |
    grep -v 'img/README' |
    grep -v 'img/mkicon.sh' |
    grep -v '\.bat$' |
    sort > install/manifest.txt

systype=$(uname -s)
case $systype in
  Darwin)
    cat install/manifest.txt |
      sed -e 's,\.so$,.dylib,' \
      > install/manifest.n
    mv -f install/manifest.n install/manifest
    ;;
  MSYS*|MINGW**)
    cat install/manifest.txt |
      sed -e 's,bin/bdj4\([^\.]*\),bin/bdj4\1.exe,' \
          -e 's,\.so$,.dll,' \
          -e 's,\.sh$,.bat,' \
      > install/manifest.n
    mv -f install/manifest.n install/manifest
    ;;
esac
