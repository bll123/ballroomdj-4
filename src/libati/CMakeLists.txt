#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4atiutil SHARED
  atiutil.c
  atioggutil.c
)
target_include_directories (libbdj4atiutil
  PRIVATE ${PKG_LIBOPUS_INCLUDE_DIRS} ${PKG_LIBOPUSFILE_INCLUDE_DIRS}
)
target_link_libraries (libbdj4atiutil PRIVATE
  # ogg
  ${PKG_LIBVORBISFILE_LDFLAGS}
  ${PKG_LIBVORBIS_LDFLAGS}
  ${PKG_LIBOGG_LDFLAGS}
  # opus
  ${PKG_LIBOPUSFILE_LDFLAGS}
  ${PKG_LIBOPUS_LDFLAGS}
  libbdj4basic libbdj4common
)

add_library (libbdj4ati SHARED
  ati.c
)
target_include_directories (libbdj4ati
  PRIVATE ${PKG_LIBOPUS_INCLUDE_DIRS} ${PKG_LIBOPUSFILE_INCLUDE_DIRS}
)
target_link_libraries (libbdj4ati PRIVATE
  # ogg
  ${PKG_LIBVORBISFILE_LDFLAGS}
  ${PKG_LIBVORBIS_LDFLAGS}
  ${PKG_LIBOGG_LDFLAGS}
  # opus
  ${PKG_LIBOPUSFILE_LDFLAGS}
  ${PKG_LIBOPUS_LDFLAGS}
  libbdj4atiutil
  libbdj4basic libbdj4common
)

add_library (libatibdj4 SHARED
  atibdj4.c
  atibdj4flac.c
  atibdj4mp3.c
  atibdj4ogg.c
  atibdj4opus.c
  atibdj4mp4.c
  atibdj4asf.c
  atibdj4riff.c
)
target_include_directories (libatibdj4
  PRIVATE ${PKG_LIBOPUS_INCLUDE_DIRS}
  PRIVATE ${PKG_LIBOPUSFILE_INCLUDE_DIRS}
)
target_link_libraries (libatibdj4 PRIVATE
  # put this first, otherwise the wrong libid3tag library is picked up
  # libid3tag
  ${PKG_LIBID3TAG_LDFLAGS}
  # libmp4tag
  ${PKG_LIBMP4TAG_LDFLAGS}
  # vorbis
  ${PKG_LIBVORBISFILE_LDFLAGS}
  ${PKG_LIBVORBIS_LDFLAGS}
  ${PKG_LIBOGG_LDFLAGS}
  # opus
  ${PKG_LIBOPUSFILE_LDFLAGS}
  ${PKG_LIBOPUS_LDFLAGS}
  # flac
  ${PKG_LIBFLAC_LDFLAGS}
  libbdj4atiutil
  libbdj4basic
  libbdj4common
)
addIntlLibrary (libatibdj4)
addWinSockLibrary (libatibdj4)

# if another ati library is added, be sure to update the installer.
# the ati interface names are hard coded there.
install (TARGETS
  libbdj4ati
  libbdj4atiutil
  libatibdj4
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4ati)
updateRPath (libbdj4atiutil)
updateRPath (libatibdj4)
