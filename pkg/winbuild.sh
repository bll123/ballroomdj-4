#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
# 2023-5-5
#   Could not get curl to link with libressl, so I just removed libressl.
#   This means that the msys2 openssl libraries and dependencies get shipped.
#   There may be some issue in the way libressl is being built.
#
# windows

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd packages
cwd=$(pwd)

comp=cc
noclean=F
noconf=F
bits=64
pkgname=""

while test $# -gt 0;do
  case $1 in
    --noclean)
      noclean=T
      ;;
    --noconf)
      noconf=T
      ;;
    --pkg)
      shift
      pkgname=$1
      ;;
    *)
      ;;
  esac
  shift
done

cwd=$(pwd)
INSTLOC=$cwd/../plocal
test -d $INSTLOC || mkdir -p $INSTLOC

CC=$comp
export CC
args=""
if [ "$MSYSTEM" = "" ]; then
  MSYSTEM=$bits
fi

case $MSYSTEM in
  *32)
    echo "32 bit platform not supported"
    exit 1
    ;;
  *64)
    b=64
    CFLAGS=-m64
    LDFLAGS=-m64
    PKG_CFLAGS=-m64
    export CFLAGS PKG_CFLAGS LDFLAGS
    ;;
esac

# if an older compiler is needed...
# if [ -d /winlibs/mingw${b}/bin ]; then
#   PATH=/winlibs/mingw${b}/bin:$PATH
# fi

echo "## gcc version: $(gcc --version | head -1)"

procs=$(getconf _NPROCESSORS_ONLN)
echo "= procs: $procs"

export PKG_CONFIG_PATH=$INSTLOC/lib/pkgconfig

if [[ $pkgname == "" || $pkgname = "check" ]]; then
  cd $cwd
  cd check*
  if [ $? -eq 0 ]; then
    echo "## build check"
    touch configure
    if [ $noclean = F ]; then
      make distclean
    fi
    ./configure --prefix=$INSTLOC --disable-static --disable-subunit
    make -j $procs
    # makeinfo is not present, so use -k
    make -k install
  fi
fi

if [[ $pkgname == "" || $pkgname = "nghttp2" ]]; then
  cd $cwd
  cd nghttp2*
  if [ $? -eq 0 ]; then
    echo "## build nghttp2"
    if [ $noclean = F ]; then
      make distclean
    fi
    if [ $noconf = F ]; then
      ./configure $args \
          --prefix=$INSTLOC \
          --enable-lib-only
    fi
    if [ $noclean = F ]; then
      make clean
    fi
    make -j $procs LIBS="-static-libgcc -static-libstdc++"
    make installdirs
    make install-exec
    make -C lib/includes install-nobase_includeHEADERS
  fi
fi

if [[ $pkgname == "" || $pkgname = "curl" ]]; then
  cd $cwd
  cd curl-*
  if [ $? -eq 0 ]; then
    echo "## build curl"

    if [ $noclean = F ]; then
      make -k distclean
      # make distclean does not always work well.
      make clean
      test -f config.cache && rm -f config.cache
    fi
    if [ $noconf = F ]; then
      ./configure $args \
          --enable-shared \
          --prefix=$INSTLOC \
          --with-zlib \
          --with-zstd \
          --with-openssl \
          --enable-sspi \
          --with-nghttp2=$(pwd)/../../plocal \
          --with-winidn \
          --enable-ares \
          --enable-ipv6 \
          --disable-pthreads \
          --disable-static \
          --disable-file \
          --disable-ftp \
          --disable-ldap \
          --disable-ldaps \
          --disable-dict \
          --disable-telnet \
          --disable-tftp \
          --disable-pop3 \
          --disable-imap \
          --disable-smtp \
          --disable-gopher \
          --disable-mqtt \
          --disable-manual \
          --disable-alt-svc \
          --without-amissl

      sleep 2
      find . -name '*.Plo' -print0 | xargs -0 touch
    fi
    if [ $noclean = F ]; then
      make clean
    fi
    make -j $procs LIBS="-static-libgcc"
    make install
  fi
fi

if [[ $pkgname == "" || $pkgname = "libid3tag" ]]; then
  cd $cwd
  cd libid3tag*

  if [ $? -eq 0 ]; then
    echo "## build libid3tag"
    sdir=$(pwd)
    bdir=$(pwd)/build

    fn=CMakeLists.txt
    if [[ ! -f ${fn}.orig ]]; then
      cp -f ${fn} ${fn}.orig
    fi
    cp -f ${fn}.orig ${fn}
    sed -i -e 's/^cmake_dep/# cmake_dep/' -e '/^set..TARGET/ s/libid3tag/id3tag/' ${fn}

    if [ $noclean = F ]; then
      test -d build && rm -rf build
    fi
    if [ $noconf = F ]; then
      cmake -G "MSYS Makefiles" \
          -S ${sdir} \
          -B ${bdir} \
          -DBUILD_SHARED=ON \
          -DCMAKE_INSTALL_PREFIX="$INSTLOC"
    fi
    if [ $noclean = F ]; then
      cmake --build "${bdir}" --target clean
    fi
    cmake --build "${bdir}" --verbose
    cmake --install "${bdir}"
  fi
fi

if [[ $pkgname == "" || $pkgname = "libogg" ]]; then
  cd $cwd
  cd libogg*
  if [ $? -eq 0 ]; then
    echo "## build libogg"
    if [ $noclean = F ]; then
      make distclean
    fi
    if [ $noconf = F ]; then
      ./configure $args \
          --prefix=$INSTLOC \
          --disable-static
    fi
    if [ $noclean = F ]; then
      make clean
    fi
    make -j $procs LIBS="-static-libgcc -static-libstdc++"
    make install
  fi
fi

if [[ $pkgname == "" || $pkgname = "libvorbis" ]]; then
  cd $cwd
  cd libvorbis*
  if [ $? -eq 0 ]; then
    echo "## build libvorbis"
    if [ $noclean = F ]; then
      make distclean
    fi
    if [ $noconf = F ]; then
      ./configure $args \
          --prefix=$INSTLOC \
          --disable-static \
          --disable-examples
    fi
    if [ $noclean = F ]; then
      make clean
    fi
    make -j $procs LIBS="-static-libgcc -static-libstdc++"
    make install
  fi
fi

if [[ $pkgname == "" || $pkgname = "libopus" ]]; then
  cd $cwd
  cd opus-1*
  if [ $? -eq 0 ]; then
    echo "## build libopus"
    if [ $noclean = F ]; then
      make distclean
    fi
    if [ $noconf = F ]; then
      ./configure $args \
          --prefix=$INSTLOC \
          --disable-static
    fi
    if [ $noclean = F ]; then
      make clean
    fi
    make -j $procs LIBS="-static-libgcc -static-libstdc++"
    make install
  fi
fi

if [[ $pkgname == "" || $pkgname = "libopusfile" ]]; then
  cd $cwd
  cd opusfile*
  if [ $? -eq 0 ]; then
    echo "## build libopusfile"
    if [ $noclean = F ]; then
      make distclean
    fi
    if [ $noconf = F ]; then
      ./configure $args \
          --prefix=$INSTLOC \
          --disable-static \
          --disable-http \
          --disable-examples
    fi
    if [ $noclean = F ]; then
      make clean
    fi
    make -j $procs LIBS="-static-libgcc -static-libstdc++"
    make install
  fi
fi

if [[ $pkgname == "" || $pkgname = "libflac" ]]; then
  cd $cwd
  cd flac*
  if [ $? -eq 0 ]; then
    echo "## build libflac"
    if [ $noclean = F ]; then
      make distclean
    fi
    if [ $noconf = F ]; then
      ./configure $args \
          --prefix=$INSTLOC \
          --disable-static \
          --disable-programs \
          --disable-examples
    fi
    if [ $noclean = F ]; then
      make clean
    fi
    make -j $procs LIBS="-static-libgcc -static-libstdc++"
    make install
    # don't need the c++ version
    rm -f "$INSTLOC/bin/libFLAC++*" "$INSTLOC/lib/libFLAC++*" "$INSTLOC/lib/pkgconfig/flac++.pc"
  fi
fi

if [[ $pkgname == "" || $pkgname = "ffmpeg" ]]; then
  cd $cwd
  cd ffmpeg*
  if [ $? -eq 0 ]; then
    echo "## install ffmpeg"
    cp -f bin/*.dll $INSTLOC/bin
    cp -f bin/ffmpeg.exe $INSTLOC/bin
    cp -rf include/* $INSTLOC/include
  fi
fi

echo "## finalize"
find $INSTLOC -type f -print0 | xargs -0 chmod u+w
exit 0
