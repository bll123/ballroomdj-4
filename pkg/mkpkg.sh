#!/bin/bash

pkgname=BDJ4
spkgnm=bdj4
instdir=bdj4-install
# macos install needs these
pkgnameuc=$(echo ${pkgname} | tr 'a-z' 'A-Z')
pkgnamelc=$(echo ${pkgname} | tr 'A-Z' 'a-z')

function copysrcfiles {
  systype=$1
  stage=$2

  filelist="LICENSE.txt README.txt VERSION.txt
      packages/mongoose/mongoose.[ch]"
  dirlist="src conv img install licenses linux locale pkg templates web wiki"

  echo "-- copying files to $stage"
  for f in $filelist; do
    dir=$(dirname ${f})
    test -d ${stage}/${dir} || mkdir -p ${stage}/${dir}
    cp -pf ${f} ${stage}/${dir}
  done
  for d in $dirlist; do
    dir=$(dirname ${d})
    test -d ${stage}/${dir} || mkdir -p ${stage}/${dir}
    cp -pr ${d} ${stage}/${dir}
  done

  echo "   removing exclusions"
  test -d ${stage}/src/build && rm -rf ${stage}/src/build
}

function copyreleasefiles {
  systype=$1
  stage=$2

  filelist="LICENSE.txt README.txt VERSION.txt"
  dirlist="bin conv img install licenses linux locale templates"

  case ${systype} in
    Darwin)
      dirlist="$dirlist plocal/share/themes/macOS*"
      ;;
    MSYS*|MINGW*)
      dirlist="$dirlist plocal/bin plocal/share/themes/Wind*"
      dirlist="$dirlist plocal/share/icons plocal/lib/gdk-pixbuf-2.0"
      dirlist="$dirlist plocal/share/glib-2.0/schemas plocal/etc/gtk-3.0"
      dirlist="$dirlist plocal/lib/girepository-1.0 plocal/etc/fonts"
      ;;
  esac

  echo "-- copying files to $stage"
  for f in $filelist; do
    dir=$(dirname ${f})
    test -d ${stage}/${dir} || mkdir -p ${stage}/${dir}
    cp -pf ${f} ${stage}/${dir}
  done
  for d in $dirlist; do
    dir=$(dirname ${d})
    test -d ${stage}/${dir} || mkdir -p ${stage}/${dir}
    cp -pr ${d} ${stage}/${dir}
  done

  echo "   removing exclusions"
  rm -f \
      ${stage}/bin/bdj4se \
      ${stage}/bin/check_all \
      ${stage}/bin/chkprocess \
      ${stage}/img/mkicon.sh \
      ${stage}/img/README.txt \
      ${stage}/plocal/bin/checkmk \
      ${stage}/plocal/bin/curl-config \
      ${stage}/plocal/bin/libcheck-*.dll \
      ${stage}/plocal/bin/ocspcheck.exe \
      ${stage}/plocal/bin/openssl.exe
}

function setLibVol {
  dir=$1
  nlibvol=$2

  tfn="${dir}/templates/bdjconfig.txt.g"
  sed -e "s,libvolnull,${nlibvol}," "$tfn" > "${tfn}.n"
  mv -f "${tfn}.n" ${tfn}
}


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
    sfx=
    ;;
  Darwin)
    tag=macos
    platform=unix
    sfx=
    ;;
  MINGW64*)
    tag=win64
    platform=windows
    sfx=.exe
    ;;
  MINGW32*)
    tag=win32
    platform=windows
    sfx=.exe
    ;;
esac

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

# create manifests

echo "-- building software"
echo "   === currently off ==="
# (
#   cd src
#   make distclean
#   make > ../tmp/pkg-build.log 2>&1
#   make tclean > /dev/null 2>&1
# )

(cd src; make tclean > /dev/null 2>&1)

# the .po files will be built on linux; the sync to the other
# platforms must be performed afterwards.
# (the extraction script is using gnu-sed features)
if [[ $tag == linux ]]; then
  (cd src/po; ./extract.sh)
  (cd src/po; ./install.sh)
fi

# on windows, copy all of the required .dll files to plocal/bin
# this must be done after the build and before the manifest is created.

if [[ $platform == windows ]]; then

  # bll@win10-64 MINGW64 ~/bdj4/bin
  # $ ldd bdj4.exe | grep mingw
  #       libintl-8.dll => /mingw64/bin/libintl-8.dll (0x7ff88c570000)
  #       libiconv-2.dll => /mingw64/bin/libiconv-2.dll (0x7ff8837e0000)

  echo "-- copying .dll files"
  dlllistfn=tmp/dll-list.txt
  > $dlllistfn

  for fn in bin/*.exe /mingw64/bin/gdbus.exe /mingw64/bin/librsvg-2-2.dll ; do
    ldd $fn |
      grep mingw |
      sed -e 's,.*=> ,,' -e 's,\.dll .*,.dll,' >> $dlllistfn
  done
  for fn in $(sort -u $dlllistfn); do
    bfn=$(basename $fn)
    if [[ $fn -nt $bfn ]]; then
      cp -pf $fn plocal/bin
    fi
  done
  rm -f $dlllistfn

  # stage the other required gtk files.

  cp -f /mingw64/bin/librsvg-2-2.dll plocal/bin
  cp -f /mingw64/bin/gdbus.exe plocal/bin
  cp -rf /mingw64/lib/gdk-pixbuf-2.0 plocal/lib
  cp -rf /mingw64/lib/girepository-1.0 plocal/lib
  mkdir -p plocal/share/icons
  cp -rf /mingw64/share/icons/* plocal/share/icons
  mkdir -p plocal/share/glib-2.0
  cp -rf /mingw64/share/glib-2.0/schemas plocal/share/glib-2.0
  mkdir -p plocal/etc/gtk-3.0
  cp -f /mingw64/etc/gtk-3.0/im-multipress.conf plocal/etc/gtk-3.0

  # leave this here for the time being until UITHEME is implemented
  cat > plocal/etc/gtk-3.0/settings.ini <<_HERE_
[Settings]
gtk-xft-antialias = 1
gtk-enable-animations = 0
gtk-icon-theme-name = Adwaita
gtk-theme-name = Windows-10-Dark
_HERE_

  mkdir -p plocal/etc/fonts
  cp -rf /mingw64/etc/fonts plocal/etc

  # fix other windows specific stuff

  nm=templates/bdjconfig.txt.g
  sed -e 's/libvolpa/libvolwin/' ${nm} > ${nm}.n
  mv -f ${nm}.n ${nm}

  nm=templates/bdjconfig.txt.mp
  sed -e '/UIFONT/ { n ; s/.*/..Arial 11/ }' ${nm} > ${nm}.n
  mv -f ${nm}.n ${nm}

  nm=templates/bdjconfig.txt.p
  sed -e '/UITHEME/ { n ; s/.*/..Windows-10-Dark/ }' ${nm} > ${nm}.n
  mv -f ${nm}.n ${nm}
fi # is windows

if [[ $systype == Darwin ]]; then

  # fix darwin specific stuff

  nm=templates/bdjconfig.txt.g
  sed -e 's/libvolpa/libvolmac/' ${nm} > ${nm}.n
  mv -f ${nm}.n ${nm}

fi

# update build number

echo "-- updating build number"

. ./VERSION.txt

# only rebuild the version.txt file on linux.
if [[ $tag == linux ]]; then
  BUILD=$(($BUILD+1))
  BUILDDATE=$(date '+%Y%m%d')
  cat > VERSION.txt << _HERE_
VERSION=$VERSION
BUILD=$BUILD
BUILDDATE=$BUILDDATE
RELEASELEVEL=$RELEASELEVEL
_HERE_
fi

case $RELEASELEVEL in
  alpha|beta)
    rlstag=-$RELEASELEVEL
    ;;
  production)
    rlstag=""
    ;;
esac

# staging / create packags

case $systype in
  Linux)
    echo "-- create source package"
    stagedir=tmp/${spkgnm}-src
    test -d ${stagedir} && rm -rf ${stagedir}
    mkdir -p ${stagedir}
    nm=${spkgnm}-${VERSION}-src${rlstag}.tar.gz

    copysrcfiles ${systype} ${stagedir}

    echo "-- creating source manifest"
    touch ${stagedir}/install/manifest.txt
    ./pkg/mkmanifest.sh ${stagedir}
    mv -f install/manifest.txt ${stagedir}/install/manifest.txt

    (cd tmp;tar -c -z -f - $(basename $stagedir)) > ${nm}
    echo "## source package ${nm} created"
    rm -rf ${stagedir}
    ;;
esac

echo "-- create release package"

stagedir=tmp/${instdir}
test -d ${stagedir} && rm -rf ${stagedir}
mkdir -p ${stagedir}
manfn=install/manifest.txt
tmpnm=tmp/tfile.dat
tmpzip=tmp/tfile.zip
tmpsep=tmp/sep.txt
tmpmac=tmp/macos

datetag=""
if [[ $rlstag != "" ]]; then
  datetag=-$BUILDDATE
fi

nm=${spkgnm}-${VERSION}-installer-${tag}${datetag}${rlstag}${sfx}

test -d ${stagedir} && rm -rf ${stagedir}
mkdir ${stagedir}

echo -n '!~~BDJ4~~!' > ${tmpsep}

case $systype in
  Linux)
    copyreleasefiles ${systype} ${stagedir}

    echo "-- creating release manifest"
    touch ${stagedir}/${manfn}
    ./pkg/mkmanifest.sh ${stagedir}
    mv -f ${manfn} ${stagedir}/install

    setLibVol $stagedir libvolpa
    echo "-- creating install package"
    (cd tmp;tar -c -z -f - $(basename $stagedir)) > ${tmpnm}
    cat bin/bdj4se ${tmpsep} ${tmpnm} > ${nm}
    rm -f ${tmpnm}
    ;;
  Darwin)
    mkdir -p ${stagedir}/Contents/MacOS
    mkdir -p ${stagedir}/Contents/Resources
    mkdir -p ${tmpmac}
    cp -f img/BDJ4.icns ${stagedir}/Contents/Resources
    sed -e "s/#BDJVERSION#/${VERSION}/g" \
        -e "s/#BDJPKGNAME#/${pkgname}/g" \
        -e "s/#BDJUCPKGNAME#/${pkgnameuc}/g" \
        -e "s/#BDJLCPKGNAME#/${pkgnamelc}/g" \
        pkg/macos/Info.plist \
        > ${stagedir}/Contents/Info.plist
    echo -n 'APPLBDJ4' > ${stagedir}/Contents/PkgInfo
    copyreleasefiles ${systype} ${stagedir}/Contents/MacOS

    echo "-- creating release manifest"
    touch ${stagedir}/Contents/MacOS/${manfn}
    ./pkg/mkmanifest.sh ${stagedir}
    mv -f ${manfn} ${stagedir}/Contents/MacOS/install

    setLibVol $stagedir/Contents/MacOS libvolmac
    echo "-- creating install package"
    (cd tmp;tar -c -z -f - $(basename $stagedir)) > ${tmpnm}
    cat bin/bdj4se ${tmpsep} ${tmpnm} > ${nm}
    rm -f ${tmpnm}
    ;;
  MINGW64*|MINGW32*)
    copyreleasefiles ${systype} ${stagedir}

    echo "-- creating release manifest"
    touch ${stagedir}/${manfn}
    ./pkg/mkmanifest.sh ${stagedir}
    mv -f ${manfn} ${stagedir}/install

    echo "-- creating install package"
    setLibVol $stagedir libvolwin
    (cd tmp; zip -r ../${tmpzip} $(basename $stagedir) ) > pkg-zip.log 2>&1
    cat bin/bdj4se.exe \
        ${tmpsep} \
        plocal/bin/miniunz.exe \
        ${tmpsep} \
        ${tmpzip} \
        > ${nm}
    rm -f ${tmpzip} ${tmpsep}
    ;;
esac
chmod a+rx ${nm}

echo "## release package ${nm} created"
rm -rf ${stagedir}

exit 0
