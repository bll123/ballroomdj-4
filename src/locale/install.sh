#!/bin/bash

rm -rf ../../locale

for i in *.po; do 
  j=$(echo $i | sed 's,.po$,,')
  mkdir -p ../../locale/$j/LC_MESSAGES
  msgfmt -o ../../locale/$j/LC_MESSAGES/bdj.mo $i
done
