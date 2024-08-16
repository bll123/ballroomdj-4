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

if (NOT WIN32 AND NOT APPLE)
  add_library (libcontmpris SHARED mprisi.c)
  target_include_directories (libcontmpris
    PRIVATE "${GLIB_INCLUDE_DIRS}"
  )
  target_link_libraries (libcontmpris PRIVATE
    libbdj4basic libbdj4common
    objmprisroot objmprisplayer objdbusi
    ${GIO_LDFLAGS}
  )
endif()

if (WIN32)
  add_library (libcontsmtc SHARED smtc.cpp)
  if (CMAKE_COMPILER_IS_GNUCXX AND
      CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 10 AND
      CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11)
    target_compile_options (libcontsmtc PRIVATE
      -fcoroutines
    )
  endif()
  target_link_libraries (libcontsmtc PRIVATE
    libbdj4basic libbdj4common
    # the windowapp library is an umbrella library for winrt.
    windowsapp
  )
endif()

install (TARGETS
  libbdj4cont
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (NOT WIN32 AND NOT APPLE)
  install (TARGETS
    libcontmpris
    DESTINATION ${PROJECT_SOURCE_DIR}/../bin
  )
endif()

if (WIN32)
  install (TARGETS
    libcontsmtc
    DESTINATION ${PROJECT_SOURCE_DIR}/../bin
  )
endif()

updateRPath (libbdj4cont)
if (NOT WIN32 AND NOT APPLE)
  updateRPath (libcontmpris)
endif()
if (WIN32)
  updateRPath (libcontsmtc)
endif()
