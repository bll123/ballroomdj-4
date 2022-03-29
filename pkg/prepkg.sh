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

./src/utils/makehtmllist.sh

cp -f templates/*.svg img

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

exit 0
