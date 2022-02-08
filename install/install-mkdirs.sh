#!/bin/bash

LOG=""

export newinstall
export guienabled
export targetdir
export unpackdir
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

if [[ ! -d $targetdir ]]; then
  mkdir -p "$targetdir"
fi

# only the directories that are not in the archive need to be created

hostname=$(hostname)
if [[ $hostname == "" ]]; then
  hostname=$(uname -n)
fi

dirlist="data tmp data/profiles data/$hostname data/$hostname/profiles"

for d in $dirlist; do
  td="$targetdir/$d"
  test -d "$td" || mkdir "$td"
done

exit 0
