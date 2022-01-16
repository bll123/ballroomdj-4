#!/bin/bash

systype=$(uname -s)
case $systype in
  Linux)
    find LICENSE.txt README.txt VERSION.txt \
          src conv img install licenses locale pkg mkconfig templates web \
          packages/mongoose[0-9]*/mongoose.[ch] \
          -type f -print |
        sort > install/src-manifest.txt
    ;;
esac

