#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4audiosrc SHARED
  audiosrc.c
)
target_link_libraries (libbdj4audiosrc PUBLIC
  libdylib
  libdyintfc
  libbdj4common
  m
)

add_library (libaudiosrcfile SHARED audiosrcfile.c)
target_link_libraries (libaudiosrcfile PUBLIC
  libbdj4common
  m
)

install (TARGETS
  libbdj4audiosrc
  libaudiosrcfile
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libbdj4audiosrc)
updateRPath (libaudiosrcfile)
