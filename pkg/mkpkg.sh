#!/bin/bash

function copyfiles (
  manifest=$1
  stage=$2

  echo "-- copying files using $manifest to $stage"
  for fn in $(cat $manifest); do
    dir=${stage}/$(dirname $fn)
    test -d $dir || mkdir -p $dir
    cp -pf $fn ${dir}
  done
)

# setup

cwd=$(pwd)
case $cwd in
  */bdj4)
    ;;
  */bdj4/*)
    cwd=$(dirname $cwd)
    while : ; do
      case $cwd in
        */bdj4)
          break
          ;;
      esac
      cwd=$(dirname $cwd)
    done
    cd $cwd
    ;;
esac

systype=$(uname -s)
case $systype in
  Linux)
    tag=linux
    platform=unix
    sfx=.run
    ;;
  Darwin)
    tag=macos
    platform=unix
    sfx=.run
    ;;
  MINGW64)
    tag=win64
    platform=windows
    sfx=.exe
    ;;
  MINGW32)
    tag=win32
    platform=windows
    sfx=.exe
    ;;
esac

echo "-- checking"

instpfx=install/install-prefix.sh
sza=$(stat --format '%s' ${instpfx})
szb=$(grep '^size=' ${instpfx} | sed -e 's/size=//')
if [[ $sza -ne $szb ]]; then
  echo "The ${instpfx} file has the wrong size set."
  exit 1
fi

echo "-- copying licenses"
licdir=licenses
rm -f ${licdir}/*
cp -f packages/mongoose/LICENSE ${licdir}/mongoose.LICENSE
if [[ $tag == windows ]]; then
  cp -f packages/libressl*/COPYING ${licdir}/libressl.LICENSE
  cp -f packages/c-ares*/LICENSE.md ${licdir}/c-ares.LICENSE.md
  cp -f packages/curl*/COPYING ${licdir}/curl.LICENSE
  cp -f packages/nghttp*/LICENSE ${licdir}/nghttp2.LICENSE
  cp -f packages/zstd*/LICENSE ${licdir}/zstd.LICENSE
fi

# on windows, copy all of the required .dll files to plocal/bin

if [[ $platform == windows ]]; then

  # bll@win10-64 MINGW64 ~/bdj4/bin
  # $ ldd bdj4.exe | grep mingw
  #       libintl-8.dll => /mingw64/bin/libintl-8.dll (0x7ff88c570000)
  #       libiconv-2.dll => /mingw64/bin/libiconv-2.dll (0x7ff8837e0000)

  echo "-- copying .dll files"
  dlllistfn=tmp/dll-list.txt
  > $dlllistfn

  for fn in bin/*.exe; do
    ldd $fn |
      grep mingw |
      sed -e 's,.*=> ,,' -e 's,\.dll .*,.dll,' >> $dlllistfn
  done
  for fn in $(sort -u $dlllistfn); do
    cp -pf $fn plocal/bin
  done
  rm -f $dlllistfn

  # stage the other required gtk files.

fi # is windows

# create manifests

echo "-- creating source manifest"
(cd src; make distclean > /dev/null 2>&1)
./pkg/mkmanifest-src.sh
echo "-- building software"
(cd src; make > ../tmp/pkg-build.log 2>&1; make tclean > /dev/null 2>&1 )
echo "-- creating release manifest"
./pkg/mkmanifest.sh

# update build number

echo "-- updating build number"

. ./VERSION.txt
BUILD=$(($BUILD+1))
cat > VERSION.txt << _HERE_
VERSION=$VERSION
BUILD=$BUILD
_HERE_

# staging / create packags

echo "-- create source package"
stagedir=tmp/bdj4-src
nm=bdj4-src-${VERSION}.tar.gz

copyfiles install/manifest-src.txt ${stagedir}
(cd tmp;tar -c -z -f - $(basename $stagedir)) > ${nm}
echo "## source package ${nm} created"
rm -rf ${stagedir}

echo "-- create release package"

stagedir=tmp/bdj4
manfn=install/manifest.txt
tmpnm=tmp/tfile.dat
tmpzip=tmp/tfile.zip
tmpsep=tmp/sep.txt

nm=bdj4-${tag}-${VERSION}-installer${sfx}

test -d ${stagedir} && rm -rf ${stagedir}
mkdir ${stagedir}

case $systype in
  Linux)
    copyfiles ${manfn} ${stagedir}
    (cd tmp;tar -c -z -f - $(basename $stagedir)) > ${tmpnm}
    cat install/install-prefix.sh ${tmpnm} > ${nm}
    rm -f ${tmpnm}
    ;;
  Darwin)
    mkdir -p ${stagedir}/Contents/MacOS
    copyfiles ${manfn} ${stagedir}/Contents/MacOS
    (cd tmp;tar -c -z -f - $(basename $stagedir)) > ${tmpnm}
    cat install/install-prefix.sh ${tmpnm} > ${nm}
    rm -f ${tmpnm}
    ;;
  MINGW64|MINGW32)
    copyfiles ${manfn} ${stagedir}
    (cd tmp; zip -r ../${tmpzip} $(basename $stagedir) )
    echo -n '!~~BDJ4~~!' > ${tmpsep}
    cat bin/bdj4se.exe ${tmpsep} \
        plocal/bin/miniunz.exe ${tmpsep} \
        ${tmpzip} > ${nm}
    rm -f ${tmpzip} ${tmpsep}
    ;;
esac
echo "## release package ${nm} created"
rm -rf ${stagedir}

exit 0
