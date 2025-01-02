#!/bin/bash
#
# Copyright 2024-2025 Brad Lanam Pleasant Hill CA

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

echo "-- $(date +%T) start"
. ./VERSION.txt

debname=ballroomdj4
rev=$(grep '^Comment: Revision:' pkg/debian/debian-control.txt | sed 's,.*: ,,')
arch=amd64
debfullnm=ballroomdj4_${VERSION}-${rev}_${arch}
DEBTOP=tmp/deb
DEBDIR=${DEBTOP}/${debfullnm}
DEBCONTROLDIR=${DEBDIR}/DEBIAN
CONTROL=${DEBCONTROLDIR}/control
BINDIR=${DEBDIR}/usr/bin
SHAREDIR=${DEBDIR}/usr/share
BDJ4DIR=${cwd}/${DEBDIR}/usr/share/${debname}
UNPACKDIR="${cwd}/tmp/bdj4-install"

test -d ${DEBTOP} && rm -rf ${DEBTOP}

mkdir -p $BINDIR
mkdir -p $SHAREDIR
mkdir -p $DEBCONTROLDIR
sed -e "s/#VERSION#/${VERSION}/" \
    -e "s/#DEBNAME#/${debname}/" \
    pkg/debian/debian-control.txt \
    > $CONTROL

echo "-- $(date +%T) creating install package"
./pkg/mkpkg.sh --preskip --insttest --clean

echo "-- $(date +%T) installing"
(
  cd "$UNPACKDIR"
  ./bin/bdj4 --bdj4installer \
      --unattended \
      --targetdir "$BDJ4DIR" \
      --unpackdir "$UNPACKDIR" \
      --readonly
)

echo "-- $(date +%T) finalizing"
(
  cd $BINDIR
  test -f bdj4 && rm -f bdj4
  ln -s ../share/${debname}/bin/bdj4 .
)

cd ${cwd}
cd ${DEBTOP}
echo "-- $(date +%T) creating .deb package"
dpkg-deb --root-owner-group --build ${debfullnm}
mv -f ${debfullnm}.deb ..

# lintian ${debfullnm}.deb

cd ${cwd}
rm -rf ${DEBTOP}

exit 0
