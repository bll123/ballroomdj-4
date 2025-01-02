#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

stagedir=$1
manfn=$2

(
  cd $stagedir
  find . -print |
    sort |
    sed -e 's,^\./,,' -e '/^\.$/d'
) > $manfn

exit 0
