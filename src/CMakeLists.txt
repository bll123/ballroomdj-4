#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

# avoid msys2/windows issue
set (CMAKE_C_COMPILER_WORKS 1)
set (CMAKE_CXX_COMPILER_WORKS 1)

set (BDJ4_VERSION 4.0.0)
project (BDJ4 VERSION ${BDJ4_VERSION})
set (BDJ4ICON bdj4_icon.ico)
set (BDJ4INSTICON bdj4_icon_inst.ico)

set (default_build_type "Debug")

set (CMAKE_MODULE_PATH utils)
set (CMAKE_SHARED_LIBRARY_PREFIX "")
set (CMAKE_STATIC_LIBRARY_PREFIX "")
set (CMAKE_INSTALL_PREFIX "")
set (CMAKE_PREFIX_PATH /opt/local;${PROJECT_SOURCE_DIR}/../plocal)

# this doesn't seem to help with locating header files...
include_directories (
  "${CMAKE_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/include"
  ${PROJECT_SOURCE_DIR}/../plocal/include
)

include_directories (
  SYSTEM /opt/local/include /usr/local/include
)

# This seems to work on linux.
# cmake uses '@rpath' on macos -- does not work.
SET (CMAKE_SKIP_BUILD_RPATH FALSE)
SET (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set (SHLIB_EXT ${CMAKE_SHARED_LIBRARY_SUFFIX})
set (MONGOOSE_INC_DIR ${PROJECT_SOURCE_DIR}/mongoose)
set (MONGOOSE_SRC ${PROJECT_SOURCE_DIR}/mongoose/mongoose.c)

# does all the configuration
include (utils/bdj4config.cmake)

include (utils/bdj4macros.cmake)

# reusable objects; there's no need to compile these twice

add_library (libbdj4string STATIC
  libcommon/bdjstring.c
  libcommon/strlcat.c
  libcommon/strlcpy.c
  libcommon/mdebug.c
)

add_library (libbdj4tmutil STATIC
  libcommon/tmutil.c
)
target_link_libraries (libbdj4tmutil
  m
)

add_library (libbdj4osutils STATIC
  libcommon/oslinuxutils.c
  libcommon/osmacutils.m
  libcommon/osprocess.c
  libcommon/osutils.c
  libcommon/oswinprocess.c
  libcommon/oswinutils.c
)
target_link_libraries (libbdj4osutils
  libbdj4tmutil
  libbdj4string
  m
)

add_library (libbdj4fileop STATIC
  libcommon/fileop.c
)
target_link_libraries (libbdj4fileop
  libbdj4osutils
  libbdj4tmutil
  libbdj4string
  m
)
add_library (libbdj4launcher STATIC
  libcommon/filedata.c
  libcommon/osnetutils.c
  libcommon/pathbld.c
  libcommon/pathutil.c
  libcommon/sysvars.c
)

# static libraries for build
add_subdirectory (libbdjregex)
add_subdirectory (libdylib)
add_subdirectory (liblocbdj3)
add_subdirectory (libwebsrv)
# shared libraries that ship
add_subdirectory (libati)
add_subdirectory (libcommon)
add_subdirectory (libbasic)
add_subdirectory (libbdj4)
add_subdirectory (libpli)
add_subdirectory (libvol)
add_subdirectory (libbdj4ui)
add_subdirectory (libuigtk3)
# executables
add_subdirectory (utility)
add_subdirectory (player)
add_subdirectory (installer)
add_subdirectory (starter)
add_subdirectory (configui)
add_subdirectory (playerui)
add_subdirectory (manageui)
# testing
add_subdirectory (check)
