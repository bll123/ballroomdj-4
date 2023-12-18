#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

# avoid msys2/windows issue
set (CMAKE_C_COMPILER_WORKS 1)
set (CMAKE_CXX_COMPILER_WORKS 1)

set (BDJ4_VERSION ${BDJ4_BUILD_VERS})
project (BDJ4 VERSION ${BDJ4_VERSION})
set (BDJ4ICON bdj4_icon.ico)
set (BDJ4INSTICON bdj4_icon_inst.ico)

set (default_build_type "Debug")

set (CMAKE_MODULE_PATH utils)
set (CMAKE_SHARED_LIBRARY_PREFIX "")
set (CMAKE_STATIC_LIBRARY_PREFIX "")
set (CMAKE_INSTALL_PREFIX "")
set (CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/../plocal;/opt/local)

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

add_library (libbdj4string OBJECT
  libcommon/bdjstring.c
  libcommon/strlcat.c
  libcommon/strlcpy.c
  libcommon/mdebug.c
)

add_library (libbdj4osutils OBJECT
  libcommon/osdirutil.c
  libcommon/osenv.c
  libcommon/osutils.c
  libcommon/oslinuxutils.c
  libcommon/osmacutils.m
  libcommon/oswinutils.c
)
target_link_libraries (libbdj4osutils PRIVATE
  libbdj4string
  m
)
addIOKitFramework (libbdj4osutils)
addIntlLibrary (libbdj4osutils)

add_library (libbdj4tmutil OBJECT
  libcommon/tmutil.c
)
target_link_libraries (libbdj4tmutil PRIVATE
  libbdj4osutils
  m
)

add_library (libbdj4osprocess OBJECT
  libcommon/osprocess.c
  libcommon/oswinprocess.c
)
target_link_libraries (libbdj4osprocess PRIVATE
  libbdj4tmutil
  libbdj4osutils
  libbdj4string
  m
)
addIOKitFramework (libbdj4osprocess)

add_library (libbdj4fileop OBJECT
  libcommon/fileop.c
)
target_link_libraries (libbdj4fileop PRIVATE
  libbdj4osutils
  libbdj4tmutil
  libbdj4string
  m
)

# the bdj4launcher library is a collection of all the other
# modules that the launcher needs that are not in any of the
# above libraries.
add_library (libbdj4launcher OBJECT
  libcommon/bdj4arg.c
  libcommon/filedata.c
  libcommon/osnetutils.c
  libcommon/pathbld.c
  libcommon/pathdisp.c
  libcommon/pathutil.c
  libcommon/sysvars.c
)

# static libraries for build
add_subdirectory (libdylib)
add_subdirectory (liblocbdj3)
add_subdirectory (libwebsrv)

# shared libraries that ship
add_subdirectory (libcommon)
add_subdirectory (libbasic)
add_subdirectory (libaudiosrc)
add_subdirectory (libbdj4)

add_subdirectory (libaudioid)
add_subdirectory (libati)
add_subdirectory (libpli)
add_subdirectory (libvol)

# executables, no gui
add_subdirectory (utility)

# ui libraries
if (BDJ4_UI STREQUAL "GTK3" OR BDJ4_UI STREQUAL "gtk3")
  add_subdirectory (libuigtk3)
endif()
if (BDJ4_UI STREQUAL "NULL" OR BDJ4_UI STREQUAL "null")
  add_subdirectory (libuinull)
endif()
add_subdirectory (libbdj4ui)

# executables
add_subdirectory (configui)
add_subdirectory (installer)
add_subdirectory (manageui)
add_subdirectory (player)
add_subdirectory (playerui)
add_subdirectory (starter)

# testing
add_subdirectory (check)
