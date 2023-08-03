#!/bin/bash
#
# ICU changes the library version number with each release.
# Because of this, the ICU library on various linux distributions are
# all different versions, and the MacPorts ICU library version gets
# changed on updates.
#
# In order to ship software linked with libicu on linux and MacOS, BDJ4
# must ship its own copy of libicu, otherwise the ICU versioning will
# prevent it from running unless there is an exact match.
#

case ${systype} in
  Linux)
    ;;
  Darwin)
    ;;
  MINGW64*)
    exit 0
    ;;
esac

if [[ $pkgname == "" || $pkgname = "icu" ]]; then
  cd $cwd
  cd icu-release*
  if [[ $? -eq 0 ]]; then
    echo ""
    echo "## build icu"

    cd icu4c/source
    chmod +x configure install-sh

    if [[ $clean == T ]]; then
      make distclean
    fi

    tfn=common/unicode/uconfig.h
    if [[ ! -f $tfn.orig ]]; then
      cp -f $tfn $tfn.orig
    fi
    cp -f $tfn.orig $tfn

    # #   define UCONFIG_ONLY_COLLATION 0
    # #   define UCONFIG_NO_LEGACY_CONVERSION 0
    sed -e '/# *define *UCONFIG_ONLY_COLLATION 0/ s,0,1,' \
        -e '/# *define *UCONFIG_NO_LEGACY_CONVERSION 0/ s,0,1,' \
        $tfn > $tfn.n
    mv -f $tfn.n $tfn

    if [[ $conf == T ]]; then
      # tools must be enabled to build the data library.
      CFLAGS="-g -O2"
      CXXFLAGS="-g -O2"
      ./configure \
          --prefix=$INSTLOC \
          --with-data-packaging=library \
          --enable-rpath \
          --disable-extras \
          --disable-icuio \
          --disable-layoutex \
          --disable-tests \
          --disable-samples
    fi

    make -j ${procs}
    make install
    (
      cd $INSTLOC
      rm -f bin/gen* bin/icu* bin/makeconv bin/pkgdata
      rm -f lib/libicutu* lib/libicutest*
      rm -rf sbin share/icu lib/icu
    )
    if [[ $platform != windows && $clean == T ]]; then
      make distclean
    fi
  fi
fi
