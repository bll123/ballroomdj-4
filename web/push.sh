#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
#

SFUSER=bll123
INSTSTAGE=$HOME/vbox_shared/bdj4inst

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

. src/utils/pkgnm.sh

systype=$(uname -s)
case $systype in
  Linux)
    tag=linux
    platform=unix
    ;;
  Darwin)
    tag=macos
    platform=unix
    ;;
  MINGW64*)
    tag=win64
    platform=windows
    ;;
  MINGW32*)
    echo "Platform not supported."
    exit 1
    ;;
esac

isprimary=F
if [[ -f devel/primary.txt ]]; then
  isprimary=T
fi

if [[ $isprimary == F ]]; then
  echo "Failed: not on primary development platform"
  exit 1
fi

. ./VERSION.txt
cvers=$(pkgcurrvers)
pnm=$(pkginstnm)
case ${pnm} in
  *-dev)
    echo "Failed: will not push development package."
    exit 1
    ;;
esac

if [[ ! -d $INSTSTAGE ]]; then
  echo "Failed: no installer staging dir"
  exit 1
fi

count=$(ls -1 $INSTSTAGE/bdj4-installer-* | grep -- "-${VERSION}" | wc -l)
# 2023-12-4 fedora failing due to some weird volume thing
# 2024-1-15 fedora dropped
# 2024-1-15 manjaro linux (arch) dropped (icu updated)
# 2024-8-11 debian-11 dropped
# 2025-8-9 debian-13 added
if [[ $count -ne 6 ]]; then
  echo "Failed: not all platforms built."
  exit 1
fi

ls -1 $INSTSTAGE/bdj4-installer-* | grep -- dev >/dev/null 2>&1
rc=$?
if [[ $rc -eq 0 ]]; then
  echo "Failed: found dev build."
  exit 1
fi

wserver=web.sourceforge.net
port=22
project=ballroomdj4
# ${remuser}@web.sourceforge.net:/home/project-web/${project}/htdocs
remuser=${SFUSER}
wwwpath=/home/project-web/${project}/htdocs
ssh="ssh -p $port"
export ssh

fn=README.md
sed -e "s~#VERSION#~${cvers}~" -e "s~#BUILDDATE#~${BUILDDATE}~" $fn > ${fn}.n
rsync -v -e ssh ${fn}.n \
    ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/${fn}
rm -f ${fn}.n

# linux scripts

fn=linux-pre-install
ver=$(install/${fn}.sh --version)
rsync -v -e ssh install/${fn}.sh \
    ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/${fn}-v${ver}.sh

fn=linux-uninstall-bdj4
ver=$(install/${fn}.sh --version)
rsync -v -e ssh install/${fn}.sh \
    ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/${fn}-v${ver}.sh

# macos scripts

fn=macos-pre-install-macports
ver=$(install/${fn}.sh --version)
rsync -v -e ssh install/${fn}.sh \
    ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/${fn}-v${ver}.sh

fn=macos-run-installer
ver=$(install/${fn}.sh --version)
rsync -v -e ssh install/${fn}.sh \
    ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/${fn}-v${ver}.sh

fn=macos-uninstall-bdj4
ver=$(install/${fn}.sh --version)
rsync -v -e ssh install/${fn}.sh \
    ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/${fn}-v${ver}.sh

# windows scripts

fn=win-uninstall-bdj4
ver=$(grep ver= install/${fn}.bat | sed -e 's,.*ver=,,')
rsync -v -e ssh install/${fn}.bat \
    ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/${fn}-v${ver}.bat

# installers

for fn in $HOME/vbox_shared/bdj4inst/bdj4-installer-*; do
  rsync -v -e ssh ${fn} \
    ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/v${VERSION}/
done

# source

spnm=$(pkgsrcnm)
rsync -v -e ssh ${spnm}.zip \
  ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/source/
rsync -v -e ssh ${spnm}.tar.gz \
  ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/source/
spnm=bdj4-src-macos${pn_datetag}.tar.gz
if [[ -f ${spnm} ]]; then
  rsync -v -e ssh ${spnm} \
    ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/source/
fi
spnm=bdj4-src-win64${pn_datetag}.zip
if [[ -f ${spnm} ]]; then
  rsync -v -e ssh ${spnm} \
    ${remuser}@frs.sourceforge.net:/home/frs/project/${project}/source/
fi

echo "## updating version file"
VERFILE=bdj4version.txt
bd=$BUILDDATE
cvers=$(pkgwebvers)
echo "$cvers" > $VERFILE
rsync -e "$ssh" -aS \
    $VERFILE ${remuser}@${wserver}:${wwwpath}
# old bdj-3 file
cvers=$VERSION
echo "$cvers" > $VERFILE
wwwpath=/home/project-web/ballroomdj/htdocs
rsync -e "$ssh" -aS \
    $VERFILE ${remuser}@${wserver}:${wwwpath}/versioncheck.txt
rm -f $VERFILE

exit 0
