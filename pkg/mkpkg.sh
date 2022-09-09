#!/bin/bash

pkgname=BDJ4
spkgnm=bdj4
instdir=bdj4-install
# macos install needs these
pkgnameuc=$(echo ${pkgname} | tr 'a-z' 'A-Z')
pkgnamelc=$(echo ${pkgname} | tr 'A-Z' 'a-z')

function copysrcfiles {
  tag=$1
  stage=$2

  filelist="ChangeLog.txt LICENSE.txt README.txt VERSION.txt
      packages/mongoose/mongoose.[ch]"
  dirlist="src conv img install licenses scripts locale pkg
      templates test-templates web wiki"

  echo "-- $(date +%T) copying files to $stage"
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
  rm -rf ${stage}/img/profile0[1-9]
}

function copyreleasefiles {
  tag=$1
  stage=$2

  filelist="ChangeLog.txt LICENSE.txt README.txt VERSION.txt"
  dirlist="bin conv img install licenses scripts locale templates"

  test -d plocal/bin || mkdir plocal/bin
  case ${tag} in
    linux)
      cp -f packages/fpcalc-${tag} plocal/bin/fpcalc
      filelist+=" plocal/bin/fpcalc"
      ;;
    macos)
      cp -f packages/fpcalc-${tag} plocal/bin/fpcalc
      filelist+=" plocal/bin/fpcalc"
      dirlist+=" plocal/share/themes/macOS*"
      ;;
    win64|win32)
      cp -f packages/fpcalc-windows.exe plocal/bin/fpcalc.exe
      filelist+=" plocal/bin/fpcalc.exe"
      dirlist+=" plocal/bin plocal/share/themes/Wind*"
      dirlist+=" plocal/share/icons plocal/lib/gdk-pixbuf-2.0"
      dirlist+=" plocal/share/glib-2.0/schemas plocal/etc/gtk-3.0"
      dirlist+=" plocal/lib/girepository-1.0 plocal/etc/fonts"
      ;;
  esac

  echo "-- $(date +%T) copying files to $stage"
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
  # bdj4se is only used for packaging
  # testing: tmusicsetup, check_all, chkprocess, voltest
  # img/profile[1-9] may be left over from testing
  rm -f \
      ${stage}/bin/bdj4se \
      ${stage}/bin/check_all \
      ${stage}/bin/chkprocess \
      ${stage}/bin/tmusicsetup \
      ${stage}/bin/voltest \
      ${state}/img/*-base.svg \
      ${stage}/img/mkicon.sh \
      ${stage}/img/README.txt \
      ${stage}/plocal/bin/checkmk \
      ${stage}/plocal/bin/libcheck-*.dll \
      ${stage}/plocal/bin/ocspcheck.exe
  rm -rf ${stage}/img/profile0[1-9]
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
  ./src/utils/mktestsetup.sh
  ./bin/bdj4 --check_all
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "pkg: tests failed"
    exit 1
  fi
  if [[ $platform == windows ]]; then
    # istring causes issues on windows when run in conjunction with
    # the other tests.  run it separately.
    CK_RUN_SUITE=istring ./bin/bdj4 --check_all
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "pkg: tests failed"
      exit 1
    fi
  fi
  ./pkg/prepkg.sh
fi

# update build number

. ./VERSION.txt

# only rebuild the version.txt file on linux.
if [[ $tag == linux ]]; then
  echo "-- $(date +%T) updating build number"
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

stagedir=tmp/${spkgnm}-src
manfn=manifest.txt
manfnpath=${stagedir}/install/${manfn}
chksumfn=checksum.txt
chksumfntmp=tmp/${chksumfn}
chksumfnpath=${stagedir}/install/${chksumfn}

case $tag in
  linux)
    echo "-- $(date +%T) create source package"
    test -d ${stagedir} && rm -rf ${stagedir}
    mkdir -p ${stagedir}
    nm=${spkgnm}-${VERSION}-src${rlstag}.tar.gz

    copysrcfiles ${tag} ${stagedir}

    echo "-- $(date +%T) creating source manifest"
    touch ${manfnpath}
    ./pkg/mkmanifest.sh ${stagedir} ${manfnpath}

    echo "-- $(date +%T) creating checksums"
    ./pkg/mkchecksum.sh ${manfnpath} ${chksumfntmp}
    mv -f ${chksumfntmp} ${chksumfnpath}

    (cd tmp;tar -c -z -f - $(basename $stagedir)) > ${nm}
    echo "## source package ${nm} created"
    rm -rf ${stagedir}
    ;;
esac

echo "-- $(date +%T) create release package"

stagedir=tmp/${instdir}

test -d ${stagedir} && rm -rf ${stagedir}
mkdir -p ${stagedir}

macosbase=""
case $tag in
  macos)
    macosbase=/Contents/MacOS
    ;;
esac

manfn=manifest.txt
manfnpath=${stagedir}${macosbase}/install/${manfn}
chksumfn=checksum.txt
chksumfntmp=tmp/${chksumfn}
chksumfnpath=${stagedir}${macosbase}/install/${chksumfn}
tmpnm=tmp/tfile.dat
tmpcab=tmp/bdj4-install.cab
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

case $tag in
  linux)
    copyreleasefiles ${tag} ${stagedir}

    echo "-- $(date +%T) creating release manifest"
    touch ${manfnpath}
    ./pkg/mkmanifest.sh ${stagedir} ${manfnpath}

    echo "-- $(date +%T) creating checksums"
    ./pkg/mkchecksum.sh ${manfnpath} ${chksumfntmp}
    mv -f ${chksumfntmp} ${chksumfnpath}

    setLibVol $stagedir libvolpa
    echo "-- $(date +%T) creating install package"
    (cd tmp;tar -c -J -f - $(basename $stagedir)) > ${tmpnm}
    cat bin/bdj4se ${tmpsep} ${tmpnm} > ${nm}
    rm -f ${tmpnm} ${tmpsep}
    ;;
  Darwin)
    mkdir -p ${stagedir}${macosbase}
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
    copyreleasefiles ${tag} ${stagedir}${macosbase}

    echo "-- $(date +%T) creating release manifest"
    touch ${manfnpath}
    ./pkg/mkmanifest.sh ${stagedir} ${manfnpath}

    echo "-- $(date +%T) creating checksums"
    ./pkg/mkchecksum.sh ${manfnpath} ${chksumfntmp}
    mv -f ${chksumfntmp} ${chksumfnpath}

    setLibVol $stagedir/${macosbase} libvolmac
    echo "-- $(date +%T) creating install package"
    (cd tmp;tar -c -J -f - $(basename $stagedir)) > ${tmpnm}
    cat bin/bdj4se ${tmpsep} ${tmpnm} > ${nm}
    rm -f ${tmpnm} ${tmpsep}
    ;;
  MINGW64*|MINGW32*)
    copyreleasefiles ${tag} ${stagedir}

    echo "-- $(date +%T) creating release manifest"
    touch ${manfnpath}
    ./pkg/mkmanifest.sh ${stagedir} ${manfnpath}

    # windows checksums are too slow to process during bdj4 installation
    # leave them off.
    if [[ 0 == 1 && $preskip == F ]]; then
      echo "-- $(date +%T) creating checksums"
      ./pkg/mkchecksum.sh ${manfnpath} ${chksumfntmp}
      mv -f ${chksumfntmp} ${chksumfnpath}
    fi

    echo "-- $(date +%T) creating install package"
    test -f $tmpcab && rm -f $tmpcab
    setLibVol $stagedir libvolwin
    (
      cd tmp;
      ../pkg/pkgmakecab.sh
    )
    if [[ ! -f $tmpcab ]]; then
      echo "ERR: no cabinet."
      exit 1
    fi
    cat bin/bdj4se.exe \
        ${tmpsep} \
        ${tmpcab} \
        > ${nm}
    rm -f ${tmpcab} ${tmpsep}
    ;;
esac

chmod a+rx ${nm}

echo "## $(date +%T) release package ${nm} created"
rm -rf ${stagedir}
# clean up copied file
rm -f plocal/bin/fpcalc*

exit 0
