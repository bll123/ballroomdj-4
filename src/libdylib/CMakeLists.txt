cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libdylib STATIC
  dylib.c
)
target_link_libraries (libdylib
  ${CMAKE_DL_LIBS}
)
