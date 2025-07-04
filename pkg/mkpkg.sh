#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

pkgname=BDJ4
spkgnm=bdj4
instdir=bdj4-install
# macos install needs these
pkgnameuc=$(echo ${pkgname} | tr 'a-z' 'A-Z')
pkgnamelc=$(echo ${pkgname} | tr 'A-Z' 'a-z')
mksrcpkg=F

function updatereadme {
  stage=$1

  echo "   updating readme"
  fn="${stage}/README.md"
  cvers=$(pkgcurrvers)
  if [[ -f ${fn} ]]; then
    sed -e "s~#VERSION#~${cvers}~" -e "s~#BUILDDATE#~${BUILDDATE}~" "${fn}" > "${fn}.n"
    mv -f "${fn}.n" "${fn}"
  fi
}

function copysrcfiles {
  tag=$1
  stage=$2

  filelist="LICENSE.txt README.md VERSION.txt BUILD.txt "
  filelist+="packages/mongoose*/LICENSE "
  dirlist="src conv img http install licenses scripts locale pkg
      templates test-templates web wiki wiki-i"

  echo "-- $(date +%T) copying files to $stage"
  for f in $filelist; do
    dir=$(dirname ${f})
    test -d ${stage}/${dir} || mkdir -p ${stage}/${dir}
    rsync -aS ${f} ${stage}/${dir}
  done
  for d in $dirlist; do
    dir=$(dirname ${d})
    test -d ${stage}/${dir} || mkdir -p ${stage}/${dir}
    rsync -aS ${d} ${stage}/${dir}
  done

  updatereadme ${stage}

  echo "   removing exclusions"
  test -d ${stage}/src/build && rm -rf ${stage}/src/build
  rm -f \
      ${stage}/http/*.bak.1 \
      ${stage}/http/curl-ca-bundle.crt \
      ${stage}/http/*.html \
      ${stage}/http/led_o*.svg
  rm -rf \
      ${stage}/img/profile0[0-9]

  dir=devel
  mkdir ${stage}/${dir}
  touch ${stage}/${dir}/srcdist.txt
}

function copyreleasefiles {
  tag=$1
  stage=$2

  filelist="LICENSE.txt README.md VERSION.txt DIST.txt "
  dirlist="bin conv http img install licenses scripts locale templates"

  if [[ ! -d plocal/bin ]];then
    mkdir -p plocal/bin
  fi
  case ${tag} in
    linux)
      if [[ -d plocal/lib ]]; then
        dirlist+=" plocal/lib"
      fi
      if [[ -d plocal/lib64 ]]; then
        dirlist+=" plocal/lib64"
      fi
      filelist+=" plocal/bin/acrcloud"
      ;;
    macos)
      dirlist+=" plocal/lib"
      dirlist+=" plocal/share/themes"
      filelist+=" plocal/bin/acrcloud"
      ;;
    win32)
      echo "Platform not supported"
      exit 1
      ;;
    win64)
      rm -rf plocal/lib/libicu*.so*
      dirlist+=" plocal/bin plocal/share/themes"
      dirlist+=" plocal/share/icons plocal/lib/gdk-pixbuf-2.0"
      dirlist+=" plocal/share/glib-2.0/schemas plocal/etc/gtk-3.0"
      dirlist+=" plocal/lib/girepository-1.0 plocal/etc/fonts"
      ;;
  esac

  echo "-- $(date +%T) copying files to $stage"
  for f in $filelist; do
    dir=$(dirname ${f})
    test -d ${stage}/${dir} || mkdir -p ${stage}/${dir}
    rsync -aS ${f} ${stage}/${dir}
  done
  for d in $dirlist; do
    dir=$(dirname ${d})
    test -d ${stage}/${dir} || mkdir -p ${stage}/${dir}
    rsync -aS ${d} ${stage}/${dir}
  done

  updatereadme ${stage}

  echo "   removing exclusions"
  # bdj4se is only used for packaging
  # testing:
  #   aesed, check_all, chkprocess, chkfileshared, dmkmfromdb
  #   tdbcompare, tdbsetval, testsuite, tmusicsetup, ttagdbchk
  #   dbustest, plisinklist, voltest, vsencdec, uitest
  # img/profile[1-9] may be left over from testing
  # 2024-1-16 do not ship the pli-mpv interface either.
  # 2024-6-3 for the time being, do not ship libplivlc4
  # 2024-8-23 for a long time, do not ship libuimacos
  rm -f \
      ${stage}/bin/aesed* \
      ${stage}/bin/bdj4se \
      ${stage}/bin/bdj4se.exe \
      ${stage}/bin/check_all* \
      ${stage}/bin/chkfileshared* \
      ${stage}/bin/chkprocess* \
      ${stage}/bin/dbustest* \
      ${stage}/bin/dmkmfromdb* \
      ${stage}/bin/libplimpv* \
      ${stage}/bin/libplinull* \
      ${stage}/bin/libplivlc4* \
      ${stage}/bin/libuimacos* \
      ${stage}/bin/libvolnull* \
      ${stage}/bin/plisinklist* \
      ${stage}/bin/tdbcompare* \
      ${stage}/bin/tdbsetval* \
      ${stage}/bin/testsuite* \
      ${stage}/bin/tmusicsetup* \
      ${stage}/bin/ttagdbchk* \
      ${stage}/bin/uitest* \
      ${stage}/bin/voltest* \
      ${stage}/bin/vsed* \
      ${stage}/http/*.bak.1 \
      ${stage}/http/curl-ca-bundle.crt \
      ${stage}/http/*.html \
      ${stage}/http/led_o*.svg \
      ${stage}/img/*-base.svg \
      ${stage}/img/mkicon.sh \
      ${stage}/img/mkmacicon.sh \
      ${stage}/img/README.txt \
      ${stage}/plocal/bin/checkmk \
      ${stage}/plocal/bin/curl-config \
      ${stage}/plocal/bin/curl.exe \
      ${stage}/plocal/bin/libcheck-*.dll \
      ${stage}/plocal/bin/libFLAC++-10.dll \
      ${stage}/plocal/bin/mp4*
  rm -rf \
      ${stage}/img/profile0[0-9] \
      ${stage}/plocal/lib/pkgconfig \
      ${stage}/plocal/lib64/pkgconfig

  # the graphics-linked launcher is not used on linux or windows
  # mpris is not available on windows or macos
  # bdj4winmksc is only for windows
  # 2024-3-9 the pli-gstreamer interface is not shipped for macos or windows.
  #   It is possible to make it work, but it has no speed+pitch control.
  case ${tag} in
    linux)
      rm -f ${stage}/bin/bdj4g
      rm -f ${stage}/bin/bdj4winmksc
      ;;
    macos)
      rm -f ${stage}/bin/libplimpris*
      rm -f ${stage}/bin/libcontmpris*   # should not exist
      rm -f ${stage}/bin/libpligst*
      rm -f ${stage}/bin/bdj4winmksc
      ;;
    win64)
      rm -f ${stage}/bin/bdj4g
      rm -f ${stage}/bin/libplimpris*
      rm -f ${stage}/bin/libcontmpris*   # should not exist
      rm -f ${stage}/bin/libpligst*
      ;;
  esac
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
pkgnmgetdata

clean=T
preskip=F
insttest=F
sourceonly=F
while test $# -gt 0; do
  case $1 in
    --source)
      mksrcpkg=T
      ;;
    --sourceonly)
      sourceonly=T
      ;;
    --preskip)
      preskip=T
      ;;
    --insttest)
      insttest=T
      ;;
    --noclean)
      clean=F
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
    case ${MSYSTEM} in
      UCRT64)
        ;;
      MINGW64)
        ;;
      CLANG64)
        ;;
    esac
    ;;
  MINGW32*)
    echo "Platform not supported"
    exit 1
    ;;
esac

isprimary=F
if [[ -f devel/primary.txt ]]; then
  isprimary=T
fi
issrcdist=F
if [[ -f devel/srcdist.txt ]]; then
  issrcdist=T
fi

DEVTMP=devel/tmp

. ./VERSION.txt

if [[ $DEVELOPMENT == "" && \
    $insttest == F && \
    $isprimary == T && \
    $tag == linux ]]; then
  mksrcpkg=T
fi

if [[ $preskip == F && $insttest == F ]]; then
  ./pkg/prepkg.sh
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "prepkg.sh failed"
    exit 1
  fi
fi

if [[ $clean == T ]]; then
  (cd src; make tclean > /dev/null 2>&1)
fi

if [[ $insttest == F && $sourceonly == F ]]; then
  # update build number

  # only rebuild the version.txt file on linux on the primary development
  # machine.
  if [[ $tag == linux ]]; then
    if [[ $isprimary == T ]]; then
      echo "-- $(date +%T) updating build number"
      BUILD=$(($BUILD+1))
      BUILDDATE=$(date '+%Y-%m-%d')
      cat > VERSION.txt << _HERE_
VERSION=$VERSION
BUILD=$BUILD
BUILDDATE=$BUILDDATE
RELEASELEVEL=$RELEASELEVEL
DEVELOPMENT=$DEVELOPMENT
_HERE_
    fi
  fi
  # always create the DIST.txt file
  cat > DIST.txt << _HERE_
DIST_TAG=${pn_dist}
_HERE_
fi

# staging / create source packages

if [[ $mksrcpkg == T && $insttest == F ]]; then
  bver=$(pkgbasevers)
  stagedir=${DEVTMP}/${spkgnm}-${bver}
  manfn=manifest.txt
  manfnpath=${stagedir}/install/${manfn}
  chksumfn=checksum.txt
  chksumfntmp=${DEVTMP}/${chksumfn}
  chksumfnpath=${stagedir}/install/${chksumfn}

  case $tag in
    linux)
      echo "-- $(date +%T) create source package"
      test -d ${stagedir} && rm -rf ${stagedir}
      mkdir -p ${stagedir}
      nm=$(pkgsrcnm)

      copysrcfiles ${tag} ${stagedir}

      f="packages/acrcloud-linux"
      dir=plocal/bin
      test -d ${stagedir}/${dir} || mkdir -p ${stagedir}/${dir}
      if [[ -f ${f} ]]; then
        rsync -aS ${f} ${stagedir}/${dir}/acrcloud
      fi

      dirlist="packages/libmp4tag* packages/libid3tag* "
      for d in $dirlist; do
        dir=$(dirname ${d})
        test -d ${stagedir}/${dir} || mkdir -p ${stagedir}/${dir}
        rsync -aS ${d} ${stagedir}/${dir}
      done

      echo "-- $(date +%T) creating source manifest"
      touch ${manfnpath}
      ./pkg/mkmanifest.sh ${stagedir} ${manfnpath}

      echo "-- $(date +%T) creating checksums"
      ./pkg/mkchecksum.sh ${manfnpath} ${chksumfntmp}
      mv -f ${chksumfntmp} ${chksumfnpath}

      (cd ${DEVTMP};tar -c -z -f ${cwd}/${nm}.tar.gz $(basename $stagedir))
      echo "## source package ${nm}.tar.gz created"
      (cd ${DEVTMP};zip -q -9 -r -o ${cwd}/${nm}.zip $(basename $stagedir))
      echo "## source package ${nm}.zip created"
      rm -rf ${stagedir}
      ;;
    macos)
      echo "-- $(date +%T) create ${tag} additional source package"
      test -d ${stagedir} && rm -rf ${stagedir}
      mkdir -p ${stagedir}
      nm=$(pkgsrcadditionalnm)

      f="packages/acrcloud-macos${pn_archtag}"
      dir=plocal/bin
      test -d ${stagedir}/${dir} || mkdir -p ${stagedir}/${dir}
      if [[ -f ${f} ]]; then
        rsync -aS ${f} ${stagedir}/${dir}/acrcloud
      fi

      dirlist="packages/icu* packages/bundles/Mojave* "
      for d in $dirlist; do
        dir=$(dirname ${d})
        test -d ${stagedir}/${dir} || mkdir -p ${stagedir}/${dir}
        rsync -aS ${d} ${stagedir}/${dir}
      done

      (cd ${DEVTMP};tar -c -z -f ${cwd}/${nm} $(basename $stagedir))
      echo "## additional source package ${nm} created"
      sourceonly=T
      rm -rf ${stagedir}
      ;;
    win64)
      echo "-- $(date +%T) create ${tag} additional source package"
      test -d ${stagedir} && rm -rf ${stagedir}
      mkdir -p ${stagedir}
      nm=$(pkgsrcadditionalnm)

      f="packages/acrcloud-win64*"
      dir=plocal/bin
      test -d ${stagedir}/${dir} || mkdir -p ${stagedir}/${dir}
      if [[ -f ${f} ]]; then
        rsync -aS ${f} ${stagedir}/${dir}/acrcloud.exe
      fi

      f="packages/fpcalc-windows.exe"
      dir=plocal/bin
      test -d ${stagedir}/${dir} || mkdir -p ${stagedir}/${dir}
      # the fpcalc executable is pre-installed in the build package.
      if [[ -f ${f} ]]; then
        rsync -aS ${f} ${stagedir}/${dir}/fpcalc.exe
      fi

      dirlist="packages/check* packages/curl* packages/ffmpeg* "
      dirlist+="packages/flac* "
      dirlist+="packages/libogg* packages/libvorbis* packages/nghttp2* "
      dirlist+="packages/opus* packages/opusfile* "
      dirlist+="packages/bundles/Windows* "
      for d in $dirlist; do
        dir=$(dirname ${d})
        test -d ${stagedir}/${dir} || mkdir -p ${stagedir}/${dir}
        rsync -aS ${d} ${stagedir}/${dir}
      done

      (cd ${DEVTMP};zip -q -9 -r -o ../${nm} $(basename $stagedir))
      echo "## additional source package ${nm} created"
      sourceonly=T

      rm -rf ${stagedir}
      ;;
  esac
fi

if [[ $sourceonly == T ]]; then
  exit 0
fi

# staging / create release package

echo "-- $(date +%T) create release package"

stagedir=${DEVTMP}/${instdir}

macosbase=""
case $tag in
  macos)
    macosbase=/Contents/MacOS
    ;;
esac

manfn=manifest.txt
manfnpath=${stagedir}${macosbase}/install/${manfn}
chksumfn=checksum.txt
chksumfntmp=${DEVTMP}/${chksumfn}
chksumfnpath=${stagedir}${macosbase}/install/${chksumfn}
tmpnm=${DEVTMP}/tfile.dat
tmpcab=${DEVTMP}/bdj4-install.cab
tmpsep=${DEVTMP}/sep.txt
tmpmac=${DEVTMP}/macos

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
      rm -f ${tmpnm} ${tmpsep}
      exit 0
    fi

    setLibVol $stagedir libvolpa
    echo "-- $(date +%T) creating install package"
    (cd ${DEVTMP};tar -c -J -f ${cwd}/${tmpnm} $(basename $stagedir))
    if [[ ! -f bin/bdj4se ]]; then
      echo "bin/bdj4se not located"
      exit 1
    fi
    cat bin/bdj4se ${tmpsep} ${tmpnm} > ${nm}
    rm -f ${tmpnm} ${tmpsep}
    ;;
  macos)
    # make sure the themes are unpacked
    if [[ ! -d plocal/share/themes/Mojave-dark-solid ||
        ! -d plocal/share/themes/Mojave-light-solid ]]; then
      ./pkg/build-all.sh --pkg themes
    fi

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
      sed -e '/UI_THEME/ { n ; s/.*/..Mojave-dark-solid/ ; }' \
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
      rm -f ${tmpnm} ${tmpsep}
      exit 0
    fi

    echo "-- $(date +%T) creating install package"
    (cd ${DEVTMP};tar -c -J -f ${cwd}/${tmpnm} $(basename $stagedir))
    if [[ ! -f bin/bdj4se ]]; then
      echo "bin/bdj4se not located"
      exit 1
    fi
    cat bin/bdj4se ${tmpsep} ${tmpnm} > ${nm}
    rm -f ${tmpnm} ${tmpsep}
    ;;
  win64)
    # make sure the themes are unpacked
    if [[ ! -d plocal/share/themes/Windows-10-Dark ||
        ! -d plocal/share/themes/Windows-10-Acrylic ]]; then
      ./pkg/build-all.sh --pkg themes
    fi

    copyreleasefiles ${tag} ${stagedir}

    tfnl=$(find ${stagedir}/templates -name bdjconfig.txt.mp)
    for tfn in ${tfnl}*; do
      sed -e '/UI_THEME/ { n ; s/.*/..Windows-10-Dark/ ; }' \
          -e '/UIFONT/ { n ; s/.*/..Arial Regular 11/ ; }' \
          -e '/LISTINGFONT/ { n ; s/.*/..Arial Regular 10/ ; }' \
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
      rm -f ${tmpnm} ${tmpsep}
      exit 0
    fi

    echo "-- $(date +%T) creating install package"
    test -f $tmpcab && rm -f $tmpcab
    (
      cd ${DEVTMP}
      ${cwd}/pkg/pkgmakecab.sh
    )
    if [[ ! -f $tmpcab ]]; then
      echo "ERR: no cabinet."
      exit 1
    fi
    if [[ ! -f bin/bdj4se.exe ]]; then
      echo "bin/bdj4se not located"
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

exit 0
