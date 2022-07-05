#!/bin/bash

stagedir=$1
manfn=$2

(
  cd $stagedir
  find . -print |
    sort |
    sed -e 's,^\./,,' -e '/^\.$/d'
) > $manfn

exit 0
