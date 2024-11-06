#!/bin/bash

# export GST_DEBUG_DUMP_DOT_DIR=/home/bll/s/bdj4/tmp/

c=0
for i in ../tmp/*-gsti-*.dot; do
  c=$(($c+1))
  tnm=$(echo $i | sed -e 's,.*-,,' -e 's,\.dot$,,')
  nm=$c-$tnm
  dot -Tpng $i -o $nm.png
done
