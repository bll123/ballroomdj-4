#!/bin/bash

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

echo "-- $(date +%T) copying licenses"
licdir=licenses
rm -f ${licdir}/*
cp -pf packages/mongoose/LICENSE ${licdir}/mongoose.LICENSE
if [[ $tag == windows ]]; then
  cp -pf packages/libressl*/COPYING ${licdir}/libressl.LICENSE
  cp -pf packages/c-ares*/LICENSE.md ${licdir}/c-ares.LICENSE.md
  cp -pf packages/curl*/COPYING ${licdir}/curl.LICENSE
  cp -pf packages/nghttp*/LICENSE ${licdir}/nghttp2.LICENSE
  cp -pf packages/zstd*/LICENSE ${licdir}/zstd.LICENSE
fi

(cd src; make tclean > /dev/null 2>&1)

# the .po files will be built on linux; the sync to the other
# platforms must be performed afterwards.
# (the extraction script is using gnu-sed features)
if [[ $tag == linux ]]; then
  (cd src/po; ./extract.sh)
  (cd src/po; ./install.sh)
fi

./src/utils/makehtmllist.sh

test -d img/profile00 || mkdir -p img/profile00
cp -pf templates/*.svg img/profile00

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
  # librsvg is the SVG library
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

  for fn in bin/*.exe $exelist ; do
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
