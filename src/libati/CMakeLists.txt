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

add_library (libatiffmpeg SHARED atiffmpeg.c)
target_include_directories (libatiffmpeg
  PUBLIC "${LIBAVFORMAT_INCLUDE_DIRS}" "${LIBAVUTIL_INCLUDE_DIRS}"
)
target_link_libraries (libatiffmpeg PUBLIC
  ${LIBAVFORMAT_LDFLAGS}
  ${LIBAVUTIL_LDFLAGS}
  libbdjatiutil
  libbdj4basic
  libbdj4common
  m
)

add_library (libatimutagen SHARED atimutagen.c)
target_link_libraries (libatimutagen PUBLIC
  libbdjatiutil
  libbdj4basic
  libbdj4common
  m
)

install (TARGETS
  libbdj4ati
  libatiffmpeg
  libatimutagen
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
