#!/bin/bash

LOG=""

export newinstall
export guienabled
export targetdir
export unpackdir
export topdir
export reinstall

if [[ ! -d install ||
    ! -f install/install-helpers.sh ]]; then
  echo "Unable to locate install directory."
  exit 1
fi

. install/install-helpers.sh
processcmdargs $@

if [[ $targetdir == "" ]]; then
  echo "No target directory specified."
  exit 1
fi

for f in $(cat install/cleanuplist.txt); do
  test -f $f && rm -f $f
  test -d $f && rm -rf $f
done

exit 0
