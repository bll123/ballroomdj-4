#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

# avoid msys2/windows issue
set (CMAKE_C_COMPILER_WORKS 1)
set (CMAKE_CXX_COMPILER_WORKS 1)

set (BDJ4_VERSION ${BDJ4_BUILD_VERS})
project (BDJ4
  VERSION ${BDJ4_VERSION}
  DESCRIPTION "BallroomDJ 4"
  HOMEPAGE_URL "https://ballroomdj4.sourceforge.io"
  LANGUAGES C CXX OBJC
)
set (BDJ4ICON bdj4_icon.ico)
set (BDJ4INSTICON bdj4_icon_inst.ico)

set (default_build_type "Debug")

set (CMAKE_MODULE_PATH utils)
set (CMAKE_SHARED_LIBRARY_PREFIX "")
set (CMAKE_STATIC_LIBRARY_PREFIX "")
set (CMAKE_INSTALL_PREFIX "")
set (CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/../plocal;/opt/local)

# required for libcont/smtc.cpp
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

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

# reusable objects; there's no need to compile these multiple times
# bdj4se and bdj4 need to be linked as static objects

# the following are needed by both bdj4se and bdj4

if (WIN32)

endif()

add_library (objosprocess OBJECT
  libcommon/osprocess.c
)
if (WIN32)
  target_sources (objosprocess PRIVATE libcommon/oswinprocess.c)
endif()

add_library (objosenv OBJECT
  libcommon/osenv.c
)

add_library (objdirutil OBJECT
  libcommon/osdirutil.c
)

add_library (objfileop OBJECT
  libcommon/fileop.c
)

add_library (objstring OBJECT
  libcommon/mdebug.c
  libcommon/bdjstring.c
  libcommon/stpecpy.c
)

add_library (objtmutil OBJECT
  libcommon/tmutil.c
  libcommon/strptime.c
)

# the bdj4 launcher needs certain modules to link.
# The bdj4 launcher must be able to link as a static executable
add_library (objosutils OBJECT
  libcommon/osutils.c
)
if (APPLE)
  target_sources (objosutils PRIVATE libcommon/osmacutils.m)
endif()
if (WIN32)
  target_sources (objosutils PRIVATE libcommon/oswinutils.c)
endif()
if (NOT APPLE AND NOT WIN32)
  target_sources (objosutils PRIVATE libcommon/oslinuxutils.c)
endif()

add_library (objlauncher OBJECT
  libcommon/bdj4arg.c
  libcommon/pathbld.c
  libcommon/pathdisp.c
  libcommon/sysvars.c
  libcommon/pathutil.c
  libcommon/osnetutils.c
  libcommon/filedata.c
)

# static libraries for build
add_subdirectory (libdylib)
add_subdirectory (libwebsrv)

# shared libraries that ship
add_subdirectory (libcommon)
add_subdirectory (libbasic)
add_subdirectory (libaudiosrc)
add_subdirectory (libbdj4)
add_subdirectory (libwebclient)

# dynamic shared libraries
add_subdirectory (libaudioid)
add_subdirectory (libati)
add_subdirectory (libvol)
add_subdirectory (libmpris)
add_subdirectory (libpli)
add_subdirectory (libcont)

# executables, no gui
add_subdirectory (utility)

# ui libraries
add_subdirectory (libuigeneral)
add_subdirectory (${BDJ4_UI_LIB})
add_subdirectory (libuibase)
add_subdirectory (libuiintfc)
add_subdirectory (libuicommon)
add_subdirectory (libuimanage)
add_subdirectory (libuiplayer)

# executables
add_subdirectory (configui)
add_subdirectory (installer)
add_subdirectory (manageui)
add_subdirectory (player)
add_subdirectory (playerui)
add_subdirectory (starter)
add_subdirectory (server)

# testing
add_subdirectory (check)
