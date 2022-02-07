#!/bin/bash

manfn=install/manifest-src.txt

systype=$(uname -s)
case $systype in
  Linux)
    find LICENSE.txt README.txt VERSION.txt \
          src conv img install licenses locale pkg templates web \
          packages/mongoose/mongoose.[ch] \
          -type f -print |
        sort > ${manfn}
    ;;
esac

