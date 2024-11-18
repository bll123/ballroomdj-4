#!/bin/bash
#
# Copyright 2024 Brad Lanam Pleasant Hill CA
#
#
# https://docs.github.com/en/migrations/importing-source-code/using-the-command-line-to-import-source-code/importing-a-mercurial-repository
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd ..
cwd=$(pwd)

MAINREPO=bdj4
CLONEREPO=clone-bdj4
GITREPO=git-bdj4
COMMITTERS=committers.txt

test -d $CLONEREPO && rm -rf $CLONEREPO
mkdir $CLONEREPO
(
  cd $CLONEREPO
  hg clone ../$MAINREPO .
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "hg clone failed"
    exit 1
  fi
)

test -d $GITREPO && rm -rf $GITREPO
mkdir $GITREPO
(
  cd $GITREPO
  git init
  git config core.ignoreCase false .
)

(
  cd $MAINREPO
  hg log --template "{author}\n"
) | sort | uniq > $COMMITTERS
echo '"Brad Lanam <brad.lanam.di@gmail.com>"="Brad Lanam <brad.lanam.comp@gmail.com>"' >> $COMMITTERS

unzip -q $MAINREPO/packages/bundles/fast-export-*.zip
fe=$(echo fast-export-*)

(
  cd $GITREPO
  ../${fe}/hg-fast-export.sh \
      -r ../$MAINREPO -A ../$COMMITTERS -M main .
  git remote add origin https://github.com/bll123/bdj4.git
  git push --mirror origin .
)

test -d $CLONEREPO && rm -rf $CLONEREPO
test -d $GITREPO && rm -rf $GITREPO
test -d $fe && rm -rf $fe
test -f $COMMITTERS && rm -f $COMMITTERS

exit 0
