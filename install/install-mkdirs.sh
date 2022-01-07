#!/bin/bash

for d in tmp data; do
  test -d $dir || mkdir $dir
done
exit 0

