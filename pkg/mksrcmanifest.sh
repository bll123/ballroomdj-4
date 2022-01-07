#!/bin/bash

systype=$(uname -s)
case $systype in
  Linux)
    find LICENSE.txt README.txt VERSION.txt \
          src conv img install locale pkg mkconfig templates \
          -type f -print |
        sort > install/src-manifest.txt
    ;;
esac

