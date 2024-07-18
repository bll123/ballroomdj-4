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

if (LIBVLC_FOUND)
  add_library (libplivlc SHARED plivlc.c vlci.c)
  target_compile_options (libplivlc PRIVATE -DBDJ4_VLC_VERS=3)
  target_include_directories (libplivlc
    PRIVATE "${LIBVLC_INCLUDE_DIR}"
  )
  target_link_libraries (libplivlc PRIVATE
    libbdj4common
    ${LIBVLC_LDFLAGS}
    ${LIBVLC_LIBRARY}
  )
endif()

if (LIBVLC4_FOUND)
  add_library (libplivlc4 SHARED plivlc.c vlci.c)
  target_compile_options (libplivlc4 PRIVATE -DBDJ4_VLC_VERS=4)
  target_include_directories (libplivlc4
    PRIVATE "${LIBVLC4_INCLUDE_DIR}"
  )
  target_link_libraries (libplivlc4 PRIVATE
    libbdj4common
    ${LIBVLC4_LIBRARY}
  )
endif()

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

add_library (libpligst SHARED pligst.c gsti.c)
target_include_directories (libpligst
  PRIVATE "${GST_INCLUDE_DIRS}"
  PRIVATE "${GLIB_INCLUDE_DIRS}"
)
target_link_libraries (libpligst PRIVATE
  libbdj4common
  ${GST_LDFLAGS}
  # ${GLIB_LDFLAGS}
)

install (TARGETS
  libbdj4pli
  libplinull
  libplimpris
  libpligst
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (LIBVLC_FOUND)
  install (TARGETS
    libplivlc
    DESTINATION ${PROJECT_SOURCE_DIR}/../bin
  )
  updateRPath (libplivlc)
endif()

if (LIBVLC4_FOUND)
  install (TARGETS
    libplivlc4
    DESTINATION ${PROJECT_SOURCE_DIR}/../bin
  )
  updateRPath (libplivlc4)
endif()

updateRPath (libbdj4pli)
updateRPath (libplinull)
updateRPath (libplimpris)
updateRPath (libpligst)

