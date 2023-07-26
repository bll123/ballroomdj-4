#!/bin/bash

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

echo "-- $(date +%T) start"
. ./VERSION.txt

debname=ballroomdj4
DEBTOP=tmp/deb
DEBDIR=${DEBTOP}/${debname}
DEBCONTROLDIR=${DEBDIR}/DEBIAN
CONTROL=${DEBCONTROLDIR}/control
BINDIR=${DEBDIR}/bin
SHAREDIR=${DEBDIR}/usr/share
BDJ4DIR=${cwd}/${DEBDIR}/usr/share/${debname}
UNPACKDIR="${cwd}/tmp/bdj4-install"

test -d ${DEBTOP} && rm -rf ${DEBTOP}

mkdir -p $BINDIR
mkdir -p $SHAREDIR
mkdir -p $DEBCONTROLDIR
sed -e "s/#VERSION#/${VERSION}/" \
    -e "s/#DEBNAME#/${debname}/" \
    pkg/debian-tmpl.txt \
    > $CONTROL

echo "-- $(date +%T) creating install package"
./pkg/mkpkg.sh --preskip --insttest --clean

echo "-- $(date +%T) installing"
(
  cd "$UNPACKDIR"
set -x
  ./bin/bdj4 --bdj4installer \
      --unattended \
      --nomutagen \
      --ati libatimutagen \
      --targetdir "$BDJ4DIR" \
      --unpackdir "$UNPACKDIR" \
      --musicdir "/home/bll/Music" \
      --readonly
set +x
)

echo "-- $(date +%T) finalizing"
(
  cd $BINDIR
  test -f bdj4 && rm -f bdj4
  ln -s ../usr/share/ballroomdj4/bin/bdj4 .
)

cd ${cwd}
cd ${DEBTOP}
echo "-- $(date +%T) creating .deb package"
dpkg-deb --root-owner-group --build ${debname}
# lintian ${debname}.deb

exit 0
