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

# clean up old files that are no longer in use

# if bdj3 is found, and the data file has not been populated,
# run the conversion scripts.

# copy template files over to data directory if needed.

# copy template files to http directory

# fix any permissions.
