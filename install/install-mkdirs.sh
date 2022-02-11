#!/bin/bash

LOG=""

export topdir

if [[ ! -d install ]]; then
  echo "Unable to locate install directory."
  exit 1
fi

. install/install-helpers.sh
processcmdargs $@

if [[ $topdir == "" ]]; then
  echo "No target directory specified."
  exit 1
fi

if [[ ! -d $topdir ]]; then
  mkdir -p "$topdir"
fi

hostname=$(hostname)
if [[ $hostname == "" ]]; then
  hostname=$(uname -n)
fi

# only the directories that are not in the archive need to be created
dirlist="http tmp data data/profiles data/$hostname data/$hostname/profiles"

for d in $dirlist; do
  td="$topdir/$d"
  test -d "$td" || mkdir "$td"
done

exit 0
