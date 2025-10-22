#
# Copyright 2024-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4cont SHARED
  controller.c
)
target_link_libraries (libbdj4cont PRIVATE
  libbdj4audiosrc
  libbdj4basic libbdj4common
)

if (NOT WIN32 AND NOT APPLE)
  add_library (libcontmpris SHARED mprisi.c)
  target_include_directories (libcontmpris
    PRIVATE "${GLIB_INCLUDE_DIRS}"
  )
  target_link_libraries (libcontmpris PRIVATE
    libbdj4common
    objmprisroot objmprisplayer
    ${GIO_LDFLAGS}
  )
endif()

if (WIN32)
  add_library (libcontsmtc SHARED smtc.cpp)
  target_link_libraries (libcontsmtc PRIVATE
    libbdj4basic libbdj4common
    # the windowsapp library is an umbrella library for winrt.
    windowsapp
  )
endif()

install (TARGETS
  libbdj4cont
  DESTINATION ${DEST_BIN}
)

if (NOT WIN32 AND NOT APPLE)
  install (TARGETS
    libcontmpris
    DESTINATION ${DEST_BIN}
  )
endif()

if (WIN32)
  install (TARGETS
    libcontsmtc
    DESTINATION ${DEST_BIN}
  )
endif()

updateRPath (libbdj4cont)
if (NOT WIN32 AND NOT APPLE)
  updateRPath (libcontmpris)
endif()
if (WIN32)
  updateRPath (libcontsmtc)
endif()
