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
set (MONGOOSE_INC_DIR ${PROJECT_SOURCE_DIR}/../packages/mongoose)
set (MONGOOSE_SRC ${PROJECT_SOURCE_DIR}/../packages/mongoose/mongoose.c)

# does all the configuration
include (utils/bdj4config.cmake)

include (utils/bdj4macros.cmake)

# reusable objects; there's no need to compile these twice

add_library (libbdj4string STATIC
  libcommon/bdjstring.c
  libcommon/strlcat.c
  libcommon/strlcpy.c
)
add_library (libbdj4osutils STATIC
  libcommon/osutils.c
  libcommon/oslinuxutils.c
  libcommon/osmacutils.m
  libcommon/oswinutils.c
  libcommon/tmutil.c
)
target_link_libraries (libbdj4osutils
  libbdj4string
)
add_library (libbdj4fileop STATIC
  libcommon/fileop.c
)
target_link_libraries (libbdj4fileop
  libbdj4osutils
  libbdj4string
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
add_subdirectory (libcommon)
add_subdirectory (libbdj4)
add_subdirectory (libpli)
add_subdirectory (libvol)
add_subdirectory (libbdj4ui)
add_subdirectory (libuigtk3)
# executables
add_subdirectory (utility)
add_subdirectory (installer)
add_subdirectory (player)
add_subdirectory (manageui)
# testing
add_subdirectory (check)

if (WIN32)
  # the bdj4 executable
  # for cmake, must use the .o file extension, not .res
  add_custom_command (
    OUTPUT launcher.o
    COMMAND cp -f ${PROJECT_SOURCE_DIR}/../img/${BDJ4ICON} .
    COMMAND echo "id ICON ${BDJ4ICON}" > launcher.rc
    COMMAND windres launcher.rc -O coff -o launcher.o
    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/../img/${BDJ4ICON}
    BYPRODUCTS launcher.rc ${BDJ4ICON}
    VERBATIM
  )
endif()

add_executable (bdj4 bdj4.c)
if (WIN32)
  target_compile_options (bdj4 PRIVATE -municode)
  target_compile_options (bdj4 PRIVATE -mwindows)
  target_sources (bdj4 PRIVATE launcher.o)
endif()
target_link_libraries (bdj4
  libbdj4launcher
  libbdj4fileop
  libbdj4osutils
  libbdj4string
)
# for gethostname
addWinSockLibrary (bdj4)
addWinNtdllLibrary (bdj4)

# this is only used on macos
add_executable (bdj4g bdj4.c)
if (WIN32)
  target_compile_options (bdj4g PRIVATE -municode)
  target_compile_options (bdj4g PRIVATE -mwindows)
endif()
target_include_directories (bdj4g
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_compile_options (bdj4g PRIVATE -DBDJ4_GUI_LAUNCHER=1)
target_link_libraries (bdj4g
  libbdj4launcher
  libbdj4fileop
  libbdj4osutils
  libbdj4string
  ${GTK_LDFLAGS}
)
addWinSockLibrary (bdj4g)
addWinNtdllLibrary (bdj4g)

add_executable (bdj4starterui bdj4starterui.c)
if (WIN32)
  target_compile_options (bdj4starterui PRIVATE -municode)
  target_compile_options (bdj4starterui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4starterui
  libbdj4ui libuigtk3 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  z
  m
)
addIntlLibrary (bdj4starterui)
addWinSockLibrary (bdj4starterui)

add_executable (bdj4playerui bdj4playerui.c)
if (WIN32)
  target_compile_options (bdj4playerui PRIVATE -municode)
  target_compile_options (bdj4playerui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4playerui
  libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4playerui)
addWinSockLibrary (bdj4playerui)

add_executable (bdj4configui bdj4configui.c)
if (WIN32)
  target_compile_options (bdj4configui PRIVATE -municode)
  target_compile_options (bdj4configui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4configui
  libbdj4ui libuigtk3 libbdj4vol libbdj4pli libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4configui)
addWinSockLibrary (bdj4configui)

add_executable (bdj4helperui bdj4helperui.c)
if (WIN32)
  target_compile_options (bdj4helperui PRIVATE -municode)
  target_compile_options (bdj4helperui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4helperui
  libbdj4ui
  libuigtk3
  libbdj4 libbdj4common
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4helperui)
addWinSockLibrary (bdj4helperui)

install (TARGETS
  bdj4 bdj4g
  bdj4starterui
  bdj4playerui bdj4configui bdj4helperui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4)
  macUpdateRPath (bdj4g)
  macUpdateRPath (bdj4helperui)
  macUpdateRPath (bdj4starterui)
  macUpdateRPath (bdj4playerui)
  macUpdateRPath (bdj4configui)
endif()

