#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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

if (PKG_LIBVLC_FOUND)
  add_library (libplivlc SHARED plivlc.c vlci.c)
  target_compile_options (libplivlc PRIVATE -DBDJ4_VLC_VERS=3)
  target_include_directories (libplivlc
    PRIVATE "${PKG_LIBVLC_INCLUDE_DIR}"
  )
  target_link_libraries (libplivlc PRIVATE
    libbdj4common
    ${PKG_LIBVLC_LDFLAGS}
    ${PKG_LIBVLC_LIBRARY}
    m
  )
  addIntlLibrary (libplivlc)
endif()

if (PKG_LIBVLC4_FOUND)
  add_library (libplivlc4 SHARED plivlc.c vlci.c)
  target_compile_options (libplivlc4 PRIVATE -DBDJ4_VLC_VERS=4)
  target_include_directories (libplivlc4
    PRIVATE "${PKG_LIBVLC4_INCLUDE_DIR}"
  )
  target_link_libraries (libplivlc4 PRIVATE
    libbdj4common
    ${PKG_LIBVLC4_LIBRARY}
    m
  )
  addIntlLibrary (libplivlc4)
endif()

if (NOT WIN32 AND NOT APPLE)
  add_library (libplimpris SHARED plimpris.c mprisi.c)
  target_include_directories (libplimpris
    PRIVATE "${PKG_GLIB_INCLUDE_DIRS}"
  )
  target_link_libraries (libplimpris PRIVATE
    libbdj4common
    ${PKG_GIO_LDFLAGS}
  )
endif()

if (PKG_GST_FOUND)
  add_library (libpligst SHARED pligst.c gsti.c)
  target_include_directories (libpligst
    PRIVATE "${PKG_GST_INCLUDE_DIRS}"
    PRIVATE "${PKG_GLIB_INCLUDE_DIRS}"
  )
  target_link_libraries (libpligst PRIVATE
    libbdj4common
    ${PKG_GST_LDFLAGS}
    ${PKG_GLIB_LDFLAGS}
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

if (APPLE)
  add_library (libplimacosav SHARED plimacosav.c macosavi.m)
  target_link_libraries (libplimacosav PRIVATE
    "-framework Cocoa" "-framework AVFoundation" "-framework CoreMedia"
    libbdj4common
    m
  )
endif()

install (TARGETS
  libbdj4pli
  libplinull
  DESTINATION ${DEST_BIN}
)
updateRPath (libbdj4pli)
updateRPath (libplinull)

if (PKG_LIBVLC_FOUND)
  install (TARGETS
    libplivlc
    DESTINATION ${DEST_BIN}
  )
  updateRPath (libplivlc)
endif()

if (PKG_LIBVLC4_FOUND)
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

if (PKG_GST_FOUND)
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

if (APPLE)
  install (TARGETS
    libplimacosav
    DESTINATION ${DEST_BIN}
  )
  updateRPath (libplimacosav)
endif()
