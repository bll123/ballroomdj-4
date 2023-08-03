Building BDJ4

Setup
  Installing required packages:

    Linux:
      - Run:
          devel/linux-build-setup.sh

    MacOS:
      - Run:
          bdj4/install/macos-pre-install.sh
        Then:
          sudo port uninstall gtk3
          sudo port -N install gtk3-devel +quartz

    Windows:
      - Install MSYS2
      - Run:
          devel/windows-build-setup.sh

  Required source packages:
    - These I place in packages/bundles/ .
    - The fpcalc executables are copied to
      packages/fpcalc-(linux|macos|windows.exe)
    - The theme packages do not need to be unpacked.
    - The mongoose package does not beed to be unpacked.
    - The vlc package is only needed when the VLC include files
      need to be updated.  These are in libpli/vlc-<version>/ .
      The vlc windows .7z package is the only source distribution
      that has the include files.
    - All other source packages should be unpacked to packages/
    check-0.15.2.tar.gz
    chromaprint-fpcalc-1.5.1-linux-x86_64.tar.gz
    chromaprint-fpcalc-1.5.1-macos-universal.tar.gz
    chromaprint-fpcalc-1.5.1-windows-x86_64.zip
    curl-8.1.0.tar.xz
    ffmpeg-6.0-full_build-shared.zip
    flac-1.4.3.tar.xz
    icu-release-73-2.zip
    libid3tag-0.16.2.zip
    libogg-1.3.5.zip
    libvorbis-1.3.7.zip
    Mojave-dark-solid.tar.xz
    Mojave-light-solid.tar.xz
    mongoose-7.11.zip
    nghttp2-1.53.0.tar.xz
    opus-1.4.tar.gz
    opusfile-0.12.zip
    opus-tools-0.2.tar.gz
    vlc-3.0.16-win64.7z
    Windows-10-3.2.1.zip
    Windows-10-Acrylic-5.1.zip
    Windows-10-Dark-3.2.1-dark.zip
    Windows-8.1-Metro-3.0.zip

Building BDJ4:

  - Switch to the bdj4/src directory.
      cd bdj4/src
  - Build the required packages:
      ../pkg/build-all.sh
  - Build BDJ4:
      make distclean
      make
  - Package BDJ4:
      ../pkg/mkpkg.sh
      # note that the package must be built on the "primary" first.
      # to mark the linux system as a primary, create the file
      # devel/primary.txt.  Make sure that this file is not propagated
      # to the other platforms.
      # afterwards, the source tree must be synced with the other platforms
      # and built.
