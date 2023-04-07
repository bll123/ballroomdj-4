#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libdylib STATIC
  dylib.c
)
target_link_libraries (libdylib
  ${CMAKE_DL_LIBS}
)

add_library (libdyintfc STATIC
  dyintfc.c
)
target_link_libraries (libdyintfc
  libdylib
  libbdj4basic libbdj4common
  ${CMAKE_DL_LIBS}
)
