#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#

# setup

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cwd=$(pwd)

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
    echo "Platform not supported."
    exit 1
    ;;
esac

isprimary=F
if [[ -f devel/primary.txt ]]; then
  isprimary=T
fi

. src/utils/pkgnm.sh
pkgnmgetdata

# make sure the tmp dir exists
test -d tmp || mkdir tmp

maj=$(grep '^#define LIBMP4TAG_VERS_MAJOR' packages/libmp4tag*/libmp4tag.h |
  sed -e 's,.* ,,')
min=$(grep '^#define LIBMP4TAG_VERS_MINOR' packages/libmp4tag*/libmp4tag.h |
  sed -e 's,.* ,,')
rev=$(grep '^#define LIBMP4TAG_VERS_REVISION' packages/libmp4tag*/libmp4tag.h |
  sed -e 's,.* ,,')
LIBMP4TAGVER=${maj}.${min}.${rev}
if [[ ! -f plocal/lib/libmp4tag.so.${LIBMP4TAGVER} &&
    ! -f plocal/lib64/libmp4tag.so.${LIBMP4TAGVER}  &&
    ! -f plocal/bin/libmp4tag.dll &&
    ! -f plocal/lib/libmp4tag.${LIBMP4TAGVER}.dylib ]]; then
  echo "libmp4tag is not up to date"
  exit 1
fi
grep "LIBMP4TAG_VERS_REVISION ${rev}" plocal/include/libmp4tag.h > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "libmp4tag is not up to date"
  exit 1
fi

grep 'AUDIOID_START = AUDIOID_ID_ACOUSTID' src/libaudioid/audioid.c > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "audioid debugging is on"
  exit 1
fi

grep '^#define DBUS_DEBUG 0' src/libpli/dbusi.c > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "dbus debugging is on"
  exit 1
fi

grep 'ACRCLOUD_REUSE = 0,' src/libaudioid/acrcloud.c > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "acrcloud debugging is on"
  exit 1
fi

grep 'ACOUSTID_REUSE = 0,' src/libaudioid/acoustid.c > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "acoustid debugging is on"
  exit 1
fi

grep 'MUSICBRAINZ_REUSE = 0,' src/libaudioid/musicbrainz.c > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "musicbrainz debugging is on"
  exit 1
fi

for f in automatic.pldances standardrounds.pldances QueueDance.pldances dances.txt; do
  a=$(grep '^# version' templates/$f)
  b=$(grep '^# version' templates/en_US/$f)
  if [[ $a != $b ]]; then
    echo "version mismatch $f"
    exit 1
  fi
done

echo "-- $(date +%T) copying licenses"
licdir=licenses
test -d ${licdir} && rm -rf ${licdir}
mkdir -p ${licdir}
cp -pf packages/mongoose*/LICENSE ${licdir}/mongoose.LICENSE
if [[ $tag == macos ]]; then
  cp -pf packages/icu-release*/icu4c/LICENSE ${licdir}/icu.LICENSE
fi
cp -pf packages/libid3tag*/COPYING ${licdir}/libid3tag.LICENSE
cp -pf packages/libmp4tag*/LICENSE.txt ${licdir}/libmp4tag.LICENSE
if [[ $platform == windows ]]; then
  cp -pf packages/curl*/COPYING ${licdir}/curl.LICENSE
  cp -pf packages/libogg*/COPYING ${licdir}/libogg.LICENSE
  cp -pf packages/opus-1*/COPYING ${licdir}/opus.LICENSE
  cp -pf packages/opusfile*/COPYING ${licdir}/opusfile.LICENSE
  cp -pf packages/flac*/COPYING.Xiph ${licdir}/flac.LICENSE
  cp -pf packages/libvorbis*/COPYING ${licdir}/libvorbis.LICENSE
fi

(cd src; make tclean > /dev/null 2>&1)

# the .po files will be built on linux; the sync to the other
# platforms must be performed afterwards.
# (the extraction script is using gnu-sed features)
if [[ $tag == linux && $isprimary == T ]]; then
  (cd src/po; ./pobuild.sh)

  tfn=curl-ca-bundle.crt
  ttfn=templates/${tfn}
  htfn=http/${tfn}
  curl --silent --connect-timeout 3 \
      -JL --time-cond ${ttfn} \
      https://curl.se/ca/cacert.pem \
      -o ${ttfn}.n
  rc=$?
  if [[ $rc -eq 0 && -f ${ttfn}.n ]]; then
    mv -f ${ttfn}.n ${ttfn}
  fi
  if [[ ${ttfn} -nt ${htfn} ]]; then
    cp -pf ${ttfn} ${htfn}
  fi
  rm -f ${ttfn}.n

  ./src/utils/makehtmllist.sh
fi

# on windows, copy all of the required .dll files to plocal/bin
# this must be done after the build and before the manifest is created.

if [[ $platform == windows ]]; then

  # first remove any libraries that are not shipped so that
  # any dependencies are not traversed
  # 2024-3-3 pli-gst is not shipped at this time

  rm -f bin/libpligst.dll

  count=0

  # bll@win10-64 MINGW64 ~/bdj4/bin
  # $ ldd bdj4.exe | grep '(mingw|ucrt)64'
  #       libintl-8.dll => /ucrt64/bin/libintl-8.dll (0x7ff88c570000)
  #       libiconv-2.dll => /ucrt64/bin/libiconv-2.dll (0x7ff8837e0000)
  #       libiconv-2.dll => /ucrt64/bin/libiconv-2.dll (0x7ff8837e0000)

  libtag=""
  case $MSYSTEM in
    MINGW64)
      libtag=mingw64
      ;;
    UCRT64)
      libtag=ucrt64
      ;;
  esac

  if [[ $libtag == "" ]]; then
    echo "Unknown library target"
    exit 1
  fi

  echo "-- $(date +%T) copying windows files"
  PBIN=plocal/bin

  # gspawn helpers are required for the link button to work.
  # librsvg is the SVG library; it is not a direct dependent.
  # libcares is required by the libcurl build.
  # gdbus
  chkdlllist="
      /${libtag}/bin/gdbus.exe
      /${libtag}/bin/gspawn-win64-helper-console.exe
      /${libtag}/bin/gspawn-win64-helper.exe
      /${libtag}/bin/libcares-2.dll
      /${libtag}/bin/librsvg-2-2.dll
      "
  for fn in $chkdlllist; do
    bfn=$(basename $fn)
    if [[ ! -f $fn ]]; then
      echo "** $fn does not exist **"
    fi
    if [[ $fn -nt $PBIN/$bfn ]]; then
      count=$((count+1))
      echo "copying $bfn"
      cp -pf $fn $PBIN
    fi
  done

  dlllistfn=tmp/dll-list.txt
  > $dlllistfn

  for fn in plocal/bin/*.dll bin/*.exe $chkdlllist ; do
    ldd $fn |
      grep ${libtag} |
      sed -e 's,.*=> ,,' -e 's,\.dll .*,.dll,' >> $dlllistfn
  done
  for fn in $(sort -u $dlllistfn); do
    bfn=$(basename $fn)
    # libcurl and libnghttp2 are built, do not overwrite
    case ${bfn} in
      libcurl*)
        continue
        ;;
      libnghttp2*)
        continue
        ;;
    esac
    if [[ $fn -nt $PBIN/$bfn ]]; then
      count=$((count+1))
      echo "copying $bfn"
      cp -pf $fn $PBIN
    fi
  done
  rm -f $dlllistfn
  echo "   $count files copied."

  # stage the other required gtk files.

  # various gtk stuff
  rsync -aS --delete /${libtag}/lib/gdk-pixbuf-2.0 plocal/lib
  rsync -aS --delete /${libtag}/lib/girepository-1.0 plocal/lib
  mkdir -p plocal/share/icons
  rsync -aS --delete /${libtag}/share/icons/* plocal/share/icons
  mkdir -p plocal/share/glib-2.0
  rsync -aS --delete /${libtag}/share/glib-2.0/schemas plocal/share/glib-2.0
  mkdir -p plocal/etc/fonts
  rsync -aS --delete /${libtag}/etc/fonts plocal/etc
  mkdir -p plocal/etc/gtk-3.0
  cp -pf /${libtag}/etc/gtk-3.0/im-multipress.conf plocal/etc/gtk-3.0

  cat > plocal/etc/gtk-3.0/settings.ini <<_HERE_
[Settings]
gtk-xft-antialias=1
gtk-xft-hinting=1
gtk-xft-hintstyle=hintfull
gtk-enable-animations = 0
gtk-dialogs-use-header = 0
gtk-overlay-scrolling = 0
gtk-icon-theme-name = Adwaita
gtk-theme-name = Windows-10-Dark
_HERE_

  mkdir -p plocal/etc/fonts
  rsync -aS --delete /${libtag}/etc/fonts plocal/etc

fi # is windows

# create the DIST.txt file
cat > DIST.txt << _HERE_
DIST_TAG=${pn_dist}
_HERE_

exit 0
