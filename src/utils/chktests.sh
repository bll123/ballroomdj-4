#!/bin/bash

case $cwd in
  */src)
    ;;
  */bdj4)
    cd src
    ;;
esac

cd check
for f in *.c; do
  ca=$(egrep START_TEST $f| wc -l)
  cb=$(egrep END_TEST $f | wc -l)
  if [[ $ca -ne $cb ]]; then
    echo $f
  fi
done
exit 0

