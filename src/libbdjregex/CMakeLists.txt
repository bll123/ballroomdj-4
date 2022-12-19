#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdjregex STATIC
  bdjregex.c
)
target_include_directories (libbdjregex
  PUBLIC "${GLIB_INCLUDE_DIRS}"
)
target_link_libraries (libbdjregex
  ${GLIB_LDFLAGS}
)
