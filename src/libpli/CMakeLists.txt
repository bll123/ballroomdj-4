#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4pli SHARED
  pli.c
)
target_link_libraries (libbdj4pli PRIVATE
  objdylib
  libbdj4basic libbdj4common
  ${CMAKE_DL_LIBS}
)

add_library (libplinull SHARED plinull.c)
target_link_libraries (libplinull PRIVATE
  libbdj4common
)

add_library (libplivlc SHARED plivlc.c vlci.c)
target_include_directories (libplivlc
  PRIVATE "${LIBVLC_INCLUDE_DIR}"
)
target_link_libraries (libplivlc PRIVATE
  libbdj4common
  ${LIBVLC_LIBRARY}
)

# make this an object in case something else needs it
# but leave it in libpli/ for now
add_library (objdbusi OBJECT dbusi.c)
target_include_directories (objdbusi
  PRIVATE "${GIO_INCLUDE_DIRS}"
)
target_link_libraries (objdbusi PRIVATE
  ${GIO_LDFLAGS}
)

add_library (libplimpris SHARED plimpris.c mprisi.c)
target_include_directories (libplimpris
  PRIVATE "${GLIB_INCLUDE_DIRS}"
)
target_link_libraries (libplimpris PRIVATE
  libbdj4common
  objdbusi
  ${GIO_LDFLAGS}
)

install (TARGETS
  libbdj4pli
  libplinull
  libplivlc
  libplimpris
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libbdj4pli)
updateRPath (libplinull)
updateRPath (libplivlc)
updateRPath (libplimpris)
