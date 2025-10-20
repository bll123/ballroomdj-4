#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4pli SHARED
  pli.c
)
target_link_libraries (libbdj4pli PRIVATE
  libbdj4basic libbdj4common
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
  addIntlLibrary (libplivlc)
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
  addIntlLibrary (libplivlc4)
endif()

if (NOT WIN32 AND NOT APPLE)
  add_library (libplimpris SHARED plimpris.c mprisi.c)
  target_include_directories (libplimpris
    PRIVATE "${GLIB_INCLUDE_DIRS}"
  )
  target_link_libraries (libplimpris PRIVATE
    libbdj4common
    objdbusi
    ${GIO_LDFLAGS}
  )
endif()

if (GST_FOUND)
  add_library (libpligst SHARED pligst.c gsti.c)
  target_include_directories (libpligst
    PRIVATE "${GST_INCLUDE_DIRS}"
    PRIVATE "${GLIB_INCLUDE_DIRS}"
  )
  target_link_libraries (libpligst PRIVATE
    libbdj4common
    ${GST_LDFLAGS}
    ${GLIB_LDFLAGS}
  )
  addIntlLibrary (libpligst)
endif()

if (WIN32)
  add_library (libpliwinmp SHARED pliwinmp.c winmpi.cpp)
  target_link_libraries (libpliwinmp PRIVATE
    libbdj4common
    # the windowsapp library is an umbrella library for winrt.
    windowsapp
  )
  addIntlLibrary (libpliwinmp)
endif()

install (TARGETS
  libbdj4pli
  libplinull
  DESTINATION ${DEST_BIN}
)
updateRPath (libbdj4pli)
updateRPath (libplinull)

if (LIBVLC_FOUND)
  install (TARGETS
    libplivlc
    DESTINATION ${DEST_BIN}
  )
  updateRPath (libplivlc)
endif()

if (LIBVLC4_FOUND)
  install (TARGETS
    libplivlc4
    DESTINATION ${DEST_BIN}
  )
  updateRPath (libplivlc4)
endif()

if (NOT WIN32 AND NOT APPLE)
  install (TARGETS
    libplimpris
    DESTINATION ${DEST_BIN}
  )
  updateRPath (libplimpris)
endif()

if (GST_FOUND)
  install (TARGETS
    libpligst
    DESTINATION ${DEST_BIN}
  )
  updateRPath (libpligst)
endif()

if (WIN32)
  install (TARGETS
    libpliwinmp
    DESTINATION ${DEST_BIN}
  )
  updateRPath (libpliwinmp)
endif()


