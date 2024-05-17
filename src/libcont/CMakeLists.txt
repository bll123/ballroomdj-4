#
# Copyright 2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4cont SHARED
  controller.c
)
target_link_libraries (libbdj4cont PRIVATE
  objdylib
  libbdj4basic libbdj4common
  ${CMAKE_DL_LIBS}
)

add_library (libcontmpris SHARED mprisi.c)
target_link_libraries (libcontmpris PRIVATE
  libbdj4common
  objdbusi
  ${GIO_LDFLAGS}
)

install (TARGETS
  libbdj4cont
  libcontmpris
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libbdj4cont)
updateRPath (libcontmpris)
