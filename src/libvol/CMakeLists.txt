#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4vol SHARED
  volume.c
)
target_link_libraries (libbdj4vol PUBLIC
  libdyintfc
  libdylib
  libbdj4basic libbdj4common
  m
)

add_library (libvolnull SHARED volnull.c)
target_link_libraries (libvolnull PUBLIC
  libbdj4string
)

if (APPLE)
  add_library (libvolmac SHARED volmac.m)
  target_link_libraries (libvolmac PUBLIC
    "-framework Cocoa" "-framework CoreAudio"
  )
endif()

if (NOT WIN32 AND NOT APPLE)
  add_library (libvolpa SHARED volpa.c)
  if (PA_FOUND)
    target_include_directories (libvolpa
      PUBLIC "${PA_INCLUDE_DIRS}"
    )
    target_link_libraries (libvolpa PUBLIC
      ${PA_LDFLAGS}
      m
    )
  endif()
endif()

if (WIN32)
  add_library (libvolwin SHARED volwin.cpp)
  target_compile_options (libvolwin PRIVATE -static-libstdc++)
  target_link_libraries (libvolwin PUBLIC
    libbdj4fileop
    libbdj4string
  )
endif()

add_executable (voltest voltest.c)
target_link_libraries (voltest PUBLIC
  libbdj4vol libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  m
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

updateRPath (voltest)

