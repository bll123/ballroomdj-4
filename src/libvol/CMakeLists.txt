#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4vol SHARED
  volume.c
)
target_link_libraries (libbdj4vol PRIVATE
  objdylibobj
  libbdj4basic libbdj4common
  ${CMAKE_DL_LIBS}
)

add_library (libvolnull SHARED volnull.c)
target_link_libraries (libvolnull PRIVATE
  objstring
)

if (APPLE)
  add_library (libvolmac SHARED volmac.m)
  target_link_libraries (libvolmac PRIVATE
    "-framework Cocoa" "-framework CoreAudio"
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
  target_compile_options (libvolwin PRIVATE -static-libstdc++)
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
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
if (NOT WIN32 AND NOT APPLE)
  install (TARGETS
    libvolpa
    DESTINATION ${PROJECT_SOURCE_DIR}/../bin
  )
#  install (TARGETS
#    libvolpipewire
#    DESTINATION ${PROJECT_SOURCE_DIR}/../bin
#  )
endif()
if (APPLE)
  install (TARGETS
    libvolmac
    DESTINATION ${PROJECT_SOURCE_DIR}/../bin
  )
endif()
if (WIN32)
  install (TARGETS
    libvolwin
    DESTINATION ${PROJECT_SOURCE_DIR}/../bin
  )
endif()

updateRPath (libbdj4vol)
updateRPath (voltest)
