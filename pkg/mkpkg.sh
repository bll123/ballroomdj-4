#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

pkgname=BDJ4
spkgnm=bdj4
instdir=bdj4-install
# macos install needs these
pkgnameuc=$(echo ${pkgname} | tr 'a-z' 'A-Z')
pkgnamelc=$(echo ${pkgname} | tr 'A-Z' 'a-z')
mksrcpkg=F

function copysrcfiles {
  tag=$1
  stage=$2

  filelist="LICENSE.txt README.txt VERSION.txt"
  dirlist="src conv img http install licenses scripts locale pkg
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
  rm -f \
      ${stage}/http/*.bak.1 \
      ${stage}/http/curl-ca-bundle.crt \
      ${stage}/http/*.html \
      ${stage}/http/led_o*.svg
  rm -rf \
      ${stage}/img/profile0[0-9]
}

function copyreleasefiles {
  tag=$1
  stage=$2

  filelist="LICENSE.txt README.txt VERSION.txt"
  dirlist="bin conv http img install licenses scripts locale templates"

  if [[ ! -d plocal/bin ]];then
    mkdir -p plocal/bin
  fi
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
  # testing: tdbcompare, tdbdump, tmusicsetup, check_all, chkprocess,
  #   vlcsinklist, voltest, vsencdec
  # img/profile[1-9] may be left over from testing
  rm -f \
      ${stage}/bin/bdj4se \
      ${stage}/bin/check_all \
      ${stage}/bin/chkprocess \
      ${stage}/bin/tdbcompare \
      ${stage}/bin/tdbdump \
      ${stage}/bin/testsuite \
      ${stage}/bin/tmusicsetup \
      ${stage}/bin/vlcsinklist \
      ${stage}/bin/voltest \
      ${stage}/bin/vsencdec \
      ${stage}/http/*.bak.1 \
      ${stage}/http/curl-ca-bundle.crt \
      ${stage}/http/*.html \
      ${stage}/http/led_o*.svg \
      ${stage}/img/*-base.svg \
      ${stage}/img/mkicon.sh \
      ${stage}/img/mkmacicon.sh \
      ${stage}/img/README.txt \
      ${stage}/plocal/bin/checkmk \
      ${stage}/plocal/bin/curl.exe \
      ${stage}/plocal/bin/libcheck-*.dll \
      ${stage}/plocal/bin/ocspcheck.exe \
      ${stage}/plocal/bin/openssl.exe
  rm -rf \
      ${stage}/img/profile0[0-9]
}

function setLibVol {
  dir=$1
  nlibvol=$2

  tfn="${dir}/templates/bdjconfig.txt.g"
  sed -e "s,libvolnull,${nlibvol}," "$tfn" > "${tfn}.n"
  mv -f "${tfn}.n" ${tfn}
}

# setup

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

. src/utils/pkgnm.sh

preskip=F
chkskip=F
insttest=F
while test $# -gt 0; do
  case $1 in
    --preskip)
      preskip=T
      ;;
    --chkskip)
      chkskip=T
      ;;
    --insttest)
      insttest=T
      ;;
  esac
  shift
done

systype=$(uname -s)
case $systype in
  Linux)
    tag=linux
    ;;
  Darwin)
    tag=macos
    ;;
  MINGW64*)
    tag=win64
    ;;
  MINGW32*)
    tag=win32
    ;;
esac

if [[ $preskip == F && $insttest == F ]]; then
  if [[ $chkskip == F ]]; then
    ./src/utils/mktestsetup.sh --force
    ./bin/bdj4 --check_all
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "pkg: tests failed"
      exit 1
    fi
  fi
  ./pkg/prepkg.sh
fi

(cd src; make tclean > /dev/null 2>&1)

if [[ $insttest == F ]]; then
  # update build number

  . ./VERSION.txt

  # only rebuild the version.txt file on linux.
  if [[ $tag == linux ]]; then
    echo "-- $(date +%T) updating build number"
    BUILD=$(($BUILD+1))
    BUILDDATE=$(date '+%Y-%m-%d')
    cat > VERSION.txt << _HERE_
VERSION=$VERSION
BUILD=$BUILD
BUILDDATE=$BUILDDATE
RELEASELEVEL=$RELEASELEVEL
_HERE_
  fi
fi

# staging / create packages

if [[ mksrcpkg == T ]]; then
  stagedir=tmp/${spkgnm}-src
  manfn=manifest.txt
  manfnpath=${stagedir}/install/${manfn}
  chksumfn=checksum.txt
  chksumfntmp=tmp/${chksumfn}
  chksumfnpath=${stagedir}/install/${chksumfn}

  case $tag in
    linux)
      if [[ $insttest == F ]]; then
        echo "-- $(date +%T) create source package"
        test -d ${stagedir} && rm -rf ${stagedir}
        mkdir -p ${stagedir}
        nm=$(pkgsrcnm)

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
      fi
      ;;
  esac
fi

echo "-- $(date +%T) create release package"

stagedir=tmp/${instdir}

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

nm=$(pkginstnm)

test -d ${stagedir} && rm -rf ${stagedir}
mkdir -p ${stagedir}

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

    if [[ $insttest == T ]]; then
      rm -f plocal/bin/fpcalc*
      exit 0
    fi

    setLibVol $stagedir libvolpa
    echo "-- $(date +%T) creating install package"
    (cd tmp;tar -c -J -f - $(basename $stagedir)) > ${tmpnm}
    cat bin/bdj4se ${tmpsep} ${tmpnm} > ${nm}
    rm -f ${tmpnm} ${tmpsep}
    ;;
  macos)
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

    tfnl=$(find ${stagedir}${macosbase}/templates -name bdjconfig.txt.mp)
    for tfn in ${tfnl}*; do
      sed -e '/UI_THEME/ { n ; s/.*/..macOS-Mojave-dark/ ; }' \
          -e '/UIFONT/ { n ; s/.*/..Arial Regular 17/ ; }' \
          -e '/LISTINGFONT/ { n ; s/.*/..Arial Regular 16/ ; }' \
          ${tfn} > ${tfn}.n
      mv -f ${tfn}.n ${tfn}
    done
    echo "-- $(date +%T) creating release manifest"
    touch ${manfnpath}
    ./pkg/mkmanifest.sh ${stagedir} ${manfnpath}

    echo "-- $(date +%T) creating checksums"
    ./pkg/mkchecksum.sh ${manfnpath} ${chksumfntmp}
    mv -f ${chksumfntmp} ${chksumfnpath}

    setLibVol $stagedir/${macosbase} libvolmac

    if [[ $insttest == T ]]; then
      rm -f plocal/bin/fpcalc*
      exit 0
    fi

    echo "-- $(date +%T) creating install package"
    (cd tmp;tar -c -J -f - $(basename $stagedir)) > ${tmpnm}
    cat bin/bdj4se ${tmpsep} ${tmpnm} > ${nm}
    rm -f ${tmpnm} ${tmpsep}
    ;;
  win64|win32)
    copyreleasefiles ${tag} ${stagedir}

    tfnl=$(find ${stagedir}/templates -name bdjconfig.txt.mp)
    for tfn in ${tfnl}*; do
      sed -e '/UI_THEME/ { n ; s/.*/..Windows-10-Dark/ ; }' \
          -e '/UIFONT/ { n ; s/.*/..Arial Regular 14/ ; }' \
          -e '/LISTINGFONT/ { n ; s/.*/..Arial Regular 13/ ; }' \
          ${tfn} > ${tfn}.n
      mv -f ${tfn}.n ${tfn}
    done

    setLibVol $stagedir libvolwin

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

    if [[ $insttest == T ]]; then
      rm -f plocal/bin/fpcalc*
      exit 0
    fi

    echo "-- $(date +%T) creating install package"
    test -f $tmpcab && rm -f $tmpcab
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
