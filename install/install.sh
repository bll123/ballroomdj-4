#!/bin/bash

cwd=$(pwd)
case $cwd in
  */bdj4)
    ;;
  *)
    echo "run from bdj4 top level"
    exit 1
    ;;
esac

./install/install-mkdirs.sh
