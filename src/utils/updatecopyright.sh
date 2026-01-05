#!/bin/bash
#
# Copyright 2025 Brad Lanam Pleasant Hill CA
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

curryear=$(date '+%Y')

# update the list in depcheck.sh also
for fn in */*.c */*/*.c */*.cpp */*.m */*.h */ui/*.h \
    */*.sh ../*/*.sh ../pkg/*/*.sh ../pkg/windows/version.rc.in \
    CMakeLists.txt */CMakeLists.txt Makefile ../README.md ../LICENSE.txt \
    ../templates/*.css ../install/*.bat \
    po/Makefile* */*.awk config.h.in */*.cmake ../pkg/macos/*.plist; do
  case $fn in
    *src/tt.sh|*src/z.sh)
      continue
      ;;
    build/*)
      continue
      ;;
    *mongoose.[ch])
      continue
      ;;
    *strptime.c)
      # third party code
      continue
      ;;
    *mpris-root.[ch]|*mpris-player.[ch])
      # generated file
      continue
      ;;
    *potemplates.c)
      # temporary file
      continue
      ;;
    ../dev/*)
      continue
      ;;
    utils/dumpvars.cmake)
      continue
      ;;
    utils/*.sh)
      # most of this can be skipped
      continue
      ;;
  esac

  grep "Copyright.*${curryear}" $fn > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "$fn"
    grep "Copyright.*-" $fn > /dev/null 2>&1
    hasdash=$?
    if [[ $hasdash -eq 0 ]]; then
      sed -i -e "s,Copyright \([0-9]*\)-[0-9]* ,Copyright \1-${curryear} ," $fn
    else
      sed -i -e "s,Copyright \([0-9]*\) ,Copyright \1-${curryear} ," $fn
    fi
  fi
done

exit 0
