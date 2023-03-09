#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4ati SHARED
  ati.c
)
target_link_libraries (libbdj4ati
  libdylib
  libbdj4common
  m
)

add_library (libatimutagen SHARED atimutagen.c)
target_link_libraries (libatimutagen
  libbdj4basic
  libbdj4common
  m
)

add_library (libatitaglib SHARED atitaglib.cpp)
target_link_libraries (libatitaglib
  libbdj4basic
  libbdj4common
  m
)

install (TARGETS
  libbdj4ati
  libatimutagen
  libatitaglib
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

