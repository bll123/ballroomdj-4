#!/bin/bash

# export GST_DEBUG_DUMP_DOT_DIR=/home/bll/s/bdj4/tmp/

c=0
for i in ../tmp/*-gsti-*.dot; do
  c=$(($c+1))
  tc=$c
  if [[ $c -lt 10 ]]; then
    tc=0$c
  fi
  tnm=$(echo $i | sed -e 's,.*-,,' -e 's,\.dot$,,')
  nm=$tc-$tnm
  dot -Tpng $i -o $nm.png
done
