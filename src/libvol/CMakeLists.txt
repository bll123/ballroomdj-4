#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4vol SHARED
  volume.c
)
target_link_libraries (libbdj4vol PRIVATE
  objdylib
  libbdj4basic libbdj4common
  ${CMAKE_DL_LIBS}
)

add_library (libvolnull SHARED volnull.c)
target_link_libraries (libvolnull PRIVATE
  objstring
)

if (APPLE)
  add_library (libvolmac SHARED volmac.m)
  # do not know why the -framework options cannot be put into
  # a link option section.
  target_link_libraries (libvolmac PRIVATE
    "-framework Cocoa" "-framework CoreAudio"
    objstring
  )
endif()

if (NOT WIN32 AND NOT APPLE)
  if (PA_FOUND)
    add_library (libvolpa SHARED volpa.c)
    target_include_directories (libvolpa
      PRIVATE "${PA_INCLUDE_DIRS}"
    )
    target_link_libraries (libvolpa PRIVATE
      ${PA_LDFLAGS}
      objstring
    )
  endif()
endif()

if (NOT WIN32 AND NOT APPLE)
  if (PIPEWIRE_FOUND)
    add_library (libvolpipewire SHARED volpipewire.c)
    target_include_directories (libvolpipewire
      PRIVATE "${PIPEWIRE_INCLUDE_DIRS}"
    )
    target_link_libraries (libvolpipewire PRIVATE
      ${PIPEWIRE_LDFLAGS}
    )
  endif()
endif()

if (WIN32)
  add_library (libvolwin SHARED volwin.cpp)
  target_link_options (libvolwin PRIVATE -static-libstdc++)
  target_link_libraries (libvolwin PRIVATE
    objfileop
    objosutils
    objstring
  )
endif()

add_executable (voltest voltest.c)
target_link_libraries (voltest PRIVATE
  libbdj4vol libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)

install (TARGETS
  libbdj4vol libvolnull
  voltest
  DESTINATION ${DEST_BIN}
)
if (NOT WIN32 AND NOT APPLE)
  install (TARGETS
    libvolpa
    DESTINATION ${DEST_BIN}
  )
  install (TARGETS
    libvolpipewire
    DESTINATION ${DEST_BIN}
  )
endif()
if (APPLE)
  install (TARGETS
    libvolmac
    DESTINATION ${DEST_BIN}
  )
endif()
if (WIN32)
  install (TARGETS
    libvolwin
    DESTINATION ${DEST_BIN}
  )
endif()

updateRPath (libbdj4vol)
updateRPath (voltest)
