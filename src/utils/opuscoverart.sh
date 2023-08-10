#!/bin/bash

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 <in> <pic> <out>"
  exit 1
fi

in=$1
pic=$2
out=$3

if [[ ! -f $in ]]; then
  echo "No input file $in"
  exit 1
fi
if [[ ! -f $pic ]]; then
  echo "No picture file $pic"
  exit 1
fi

opusdec --force-wav "$in" - |
    opusenc --picture "$pic" - "$out"
exit $?
