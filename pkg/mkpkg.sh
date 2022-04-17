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
  dirlist="src conv img install licenses scripts locale pkg templates web wiki"

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
  dirlist="bin conv img install licenses scripts locale templates"

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
      ${stage}/bin/voltest \
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

preskip=F
if [[ $1 == "--preskip" ]]; then
  preskip=T
fi

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

if [[ $preskip == F ]]; then
  ./pkg/prepkg.sh
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
