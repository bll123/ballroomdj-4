#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libuiwcontobj OBJECT
  uiwcont.c
  uiwcontfree.c
)
target_include_directories (libuiwcontobj
  PRIVATE ${GTK_INCLUDE_DIRS}
)
