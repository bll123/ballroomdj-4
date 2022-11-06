cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4vol SHARED
  volume.c
)
target_link_libraries (libbdj4vol
  libdylib
  libbdj4common
  m
)

add_library (libvolnull SHARED volnull.c)

add_library (libvolmac SHARED volmac.m)
if (APPLE)
  target_link_libraries (libvolmac
    PUBLIC "-framework Cocoa" "-framework CoreAudio"
  )
endif()

add_library (libvolpa SHARED volpa.c)
if (PA_FOUND)
  target_include_directories (libvolpa
    PUBLIC "${PA_INCLUDE_DIRS}"
  )
  target_link_libraries (libvolpa
    ${PA_LDFLAGS}
    m
  )
endif()

add_library (libvolwin SHARED volwin.cpp)
target_compile_options (libvolwin PRIVATE -static-libstdc++)
target_link_libraries (libvolwin
  libbdj4fileop
  libbdj4string
)

add_executable (voltest voltest.c)
target_link_libraries (voltest
  libbdj4vol libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  m
)

install (TARGETS
  libbdj4vol libvolnull libvolpa libvolmac libvolwin
  voltest
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (voltest)
  macFixlibicu (voltest)
endif()

