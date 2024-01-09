#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

#
# 2023-5-5
#   Could not get curl to link with libressl, so I just removed libressl.
#   This means that the msys2 openssl libraries and dependencies get shipped.
#   There may be some issue in the way libressl is being built.
#

# dependents:
#   nghttp2

case ${systype} in
  Linux)
    exit 0
    ;;
  Darwin)
    exit 0
    ;;
  MINGW64*)
    ;;
esac

if [[ $pkgname == "" || $pkgname = "curl" ]]; then
  cd $cwd
  cd curl-*
  if [[ $? -eq 0 ]]; then
    echo ""
    echo "## build curl"

    if [[ $clean == T ]]; then
      make -k distclean
      # make distclean does not always work well.
      make clean
      test -f config.cache && rm -f config.cache
      # remove the copied c-ares library so that curl
      # will link against the proper c-ares library.
      # the c-ares library will be re-installed into plocal/bin by
      # the pre-pkg process
      rm -f $INSTLOC/bin/libcares-2.dll
    fi
    if [[ $conf == T ]]; then
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
    if [[ $clean == T ]]; then
      make clean
    fi
    make -j $procs LIBS="-static-libgcc"
    make install
    if [[ $platform != windows && $clean == T ]]; then
      make -k distclean
      # make distclean does not always work well.
      make clean
      test -f config.cache && rm -f config.cache
    fi
  fi
fi

