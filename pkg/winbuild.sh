#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
    b=32
    CFLAGS=-m32
    LDFLAGS=-m32
    PKG_CFLAGS=-m32
    export CFLAGS PKG_CFLAGS LDFLAGS
    ;;
  *64)
    b=64
    args="--enable-64bit"
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

if [[ $pkgname == "" || $pkgname = "libressl" ]]; then
  cd $cwd
  cd libressl-*
  if [ $? -eq 0 ]; then
    echo "## build libressl"
    if [ $noclean = F ]; then
      make distclean
    fi
    # make sure it is set later than configure.ac
    touch aclocal.m4
    sleep 2
    touch configure Makefile.in

    cppflags=""
    if [ $b = 32 ]; then
      cppflags=-D__MINGW_USE_VC2005_COMPAT
    fi

    if [ $noconf = F ]; then
      ./configure \
          CPPFLAGS=${cppflags} \
          CC="${CC} -static-libgcc -static-libstdc++" \
          --prefix=$INSTLOC \
          --exec-prefix=$INSTLOC \
          --disable-static \
          --disable-hardening \
          --enable-fast-install \
          --disable-dependency-tracking \
          --disable-windows-ssp
    fi
    if [ $noclean = F ]; then
      make clean
    fi
    make -j $procs
    make installdirs
    make install-pkgconfigDATA
    make install-exec
    make -C include/openssl install-data
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
      # make distclean does not work well.
      make clean
      test -f config.cache && rm -f config.cache
    fi
    if [ $noconf = F ]; then
      ./configure $args \
          --enable-shared \
          --prefix=$INSTLOC \
          --with-zlib \
          --with-zstd \
          --with-openssl=$(pwd)/../../plocal \
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

      # https://github.com/curl/curl/pull/9805/commits
      fn=src/Makefile
      if [ ! -f $fn.orig ]; then
        cp -f $fn $fn.orig
      fi
      cp -f $fn.orig $fn
      ed $fn << _HERE_
/RCFLAGS/
s/^	/	#/
a
	\$(RC) -I\$(top_srcdir)/include -DCURL_EMBED_MANIFEST \$(RCFLAGS) -i \$< -o \$@
.
w
q
_HERE_

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

if [[ $pkgname == "" || $pkgname = "ffmpeg" ]]; then
  cd $cwd
  cd ffmpeg*
  if [ $? -eq 0 ]; then
    echo "## install ffmpeg"
    cp -f bin/*.dll $INSTLOC/bin
    cp -f bin/ffmpeg.exe $INSTLOC/bin
  fi
fi

echo "## finalize"
find $INSTLOC -type f -print0 | xargs -0 chmod u+w
exit 0
