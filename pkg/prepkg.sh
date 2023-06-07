#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

echo "-- $(date +%T) copying licenses"
licdir=licenses
test -d ${licdir} && rm -rf ${licdir}
mkdir -p ${licdir}
cp -pf packages/mongoose*/LICENSE ${licdir}/mongoose.LICENSE
if [[ $tag == linux || $tag == macos ]]; then
  cp -pf packages/icu/share/icu/72.1/LICENSE ${licdir}/icu.LICENCE
fi
if [[ $platform == windows ]]; then
  cp -pf packages/curl*/COPYING ${licdir}/curl.LICENSE
  cp -pf packages/flac*/COPYING.Xiph ${licdir}/flac.LICENSE
  cp -pf packages/libid3tag*/COPYING ${licdir}/libid3tag.LICENSE
  cp -pf packages/libogg*/COPYING ${licdir}/libogg.LICENSE
  cp -pf packages/libvorbis*/COPYING ${licdir}/libvorbis.LICENSE
  cp -pf packages/opus-1*/COPYING ${licdir}/opus.LICENSE
  cp -pf packages/opusfile*/COPYING ${licdir}/opusfile.LICENSE
fi

(cd src; make tclean > /dev/null 2>&1)

# the .po files will be built on linux; the sync to the other
# platforms must be performed afterwards.
# (the extraction script is using gnu-sed features)
if [[ $tag == linux ]]; then
  (cd src/po; ./extract.sh)
  (cd src/po; ./install.sh)

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
fi

./src/utils/makehtmllist.sh

# test -d img/profile00 || mkdir -p img/profile00
# cp -pf templates/img/*.svg img/profile00

if [[ $tag == linux || $tag == macos ]]; then
  rsync -aS --delete packages/icu/lib plocal
  rm -f plocal/lib/libicutest* plocal/lib/libicutu*
  rm -rf plocal/lib/pkgconfig plocal/lib/icu
fi

# on windows, copy all of the required .dll files to plocal/bin
# this must be done after the build and before the manifest is created.

if [[ $platform == windows ]]; then

  count=0

  # bll@win10-64 MINGW64 ~/bdj4/bin
  # $ ldd bdj4.exe | grep mingw
  #       libintl-8.dll => /mingw64/bin/libintl-8.dll (0x7ff88c570000)
  #       libiconv-2.dll => /mingw64/bin/libiconv-2.dll (0x7ff8837e0000)

  echo "-- $(date +%T) copying .dll files"
  PBIN=plocal/bin
  # gspawn helpers are required for the link button to work.
  # librsvg is the SVG library; it is not a direct dependent.
  # gdbus
  exelist="
      /mingw64/bin/gspawn-win64-helper.exe
      /mingw64/bin/gspawn-win64-helper-console.exe
      /mingw64/bin/librsvg-2-2.dll
      /mingw64/bin/gdbus.exe
      "
  for fn in $exelist; do
    bfn=$(basename $fn)
    if [[ ! -f $fn ]]; then
      echo "** $fn does not exist **"
    fi
    if [[ $fn -nt $PBIN/$bfn ]]; then
      count=$((count+1))
      cp -pf $fn $PBIN
    fi
  done

  dlllistfn=tmp/dll-list.txt
  > $dlllistfn

  for fn in plocal/bin/*.dll bin/*.exe $exelist ; do
    ldd $fn |
      grep mingw |
      sed -e 's,.*=> ,,' -e 's,\.dll .*,.dll,' >> $dlllistfn
  done
  for fn in $(sort -u $dlllistfn); do
    bfn=$(basename $fn)
    if [[ $fn -nt $PBIN/$bfn ]]; then
      count=$((count+1))
      cp -pf $fn $PBIN
    fi
  done
  rm -f $dlllistfn
  echo "   $count files copied."

  # stage the other required gtk files.

  # various gtk stuff
  cp -prf /mingw64/lib/gdk-pixbuf-2.0 plocal/lib
  cp -prf /mingw64/lib/girepository-1.0 plocal/lib
  mkdir -p plocal/share/icons
  cp -prf /mingw64/share/icons/* plocal/share/icons
  mkdir -p plocal/share/glib-2.0
  cp -prf /mingw64/share/glib-2.0/schemas plocal/share/glib-2.0
  mkdir -p plocal/etc/gtk-3.0
  cp -pf /mingw64/etc/gtk-3.0/im-multipress.conf plocal/etc/gtk-3.0

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
  cp -prf /mingw64/etc/fonts plocal/etc

fi # is windows

exit 0
