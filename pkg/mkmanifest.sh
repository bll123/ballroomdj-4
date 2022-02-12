#!/bin/bash

stagedir=$1

systype=$(uname -s)

manfn=install/manifest.txt
mantmp=install/manifest.n

(
  cd $stagedir
  find . -print |
    sort |
    sed -e 's,^\./,,' -e '/^\.$/d'
) > $manfn

exit 0
