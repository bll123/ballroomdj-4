#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4ati SHARED
  ati.c
)
target_link_libraries (libbdj4ati PUBLIC
  libdylib
  libdyintfc
  libbdj4basic libbdj4common
  m
)

add_library (libbdjatiutil STATIC
  atiutil.c
)
target_link_libraries (libbdjatiutil PUBLIC
  libbdj4basic libbdj4common
  m
)

add_library (libatibdj4 SHARED
  atibdj4.c
  atibdj4flac.c
  atibdj4mp3.c
  atibdj4mp4.cpp
  atibdj4ogg.c
  atibdj4opus.c
)
target_include_directories (libatibdj4
  PUBLIC ${LIBOPUS_INCLUDE_DIRS} ${LIBOPUSFILE_INCLUDE_DIRS}
)
target_link_libraries (libatibdj4 PUBLIC
  # put this first, otherwise the wrong libid3tag library is picked up
  # libidtag
  ${LIBID3TAG_LDFLAGS}
  libbdjatiutil
  libbdj4basic
  libbdj4common
  # ffmpeg
  ${LIBAVFORMAT_LDFLAGS}
  ${LIBAVUTIL_LDFLAGS}
  # ogg
  ${LIBVORBISFILE_LDFLAGS}
  ${LIBVORBIS_LDFLAGS}
  ${LIBOGG_LDFLAGS}
  # opus
  ${LIBOPUSFILE_LDFLAGS}
  ${LIBOPUS_LDFLAGS}
  # flac
  ${LIBFLAC_LDFLAGS}
  # bento4 mp4 library
  ${LIBBENTO4_LDFLAGS}
  m
)

add_library (libatimutagen SHARED atimutagen.c)
target_link_libraries (libatimutagen PUBLIC
  libbdjatiutil
  libbdj4basic
  libbdj4common
  m
)

# if another ati library is added, be sure to update the installer.
# the ati interface names are hard coded there.
install (TARGETS
  libbdj4ati
  libatibdj4
  libatimutagen
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libatibdj4)
