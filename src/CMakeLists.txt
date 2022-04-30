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
add_library (libbdj4fileop STATIC
  libcommon/fileop.c
  libcommon/osutils.c
  libcommon/tmutil.c
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
add_subdirectory (libbdj4uigtk)

add_executable (bdj4locatebdj3 bdj4locatebdj3.c)
if (WIN32)
  target_compile_options (bdj4locatebdj3 PRIVATE -municode)
endif()
target_link_libraries (bdj4locatebdj3
  liblocatebdj3
  libbdj4fileop
  libbdj4string
)

add_executable (bdj4info bdj4info.c)
if (WIN32)
  target_compile_options (bdj4info PRIVATE -municode)
endif()
target_link_libraries (bdj4info
  libbdj4common
)
addIntlLibrary (bdj4info)

add_executable (bdj4tags bdj4tags.c)
if (WIN32)
  target_compile_options (bdj4tags PRIVATE -municode)
endif()
target_link_libraries (bdj4tags
  libbdj4common libbdj4
)
addIntlLibrary (bdj4tags)

add_executable (bdj4updater bdj4updater.c)
if (WIN32)
  target_compile_options (bdj4updater PRIVATE -municode)
endif()
if (WIN32)
  target_compile_options (bdj4updater PRIVATE -mwindows)
endif()
target_link_libraries (bdj4updater
  libbdj4common libbdj4
)
addIntlLibrary (bdj4updater)

if (WIN32)
  # the bdj4se executable
  # for cmake, must use the .o file extension, not .res
  add_custom_command (
    OUTPUT selauncher.o
    COMMAND cp -f ${PROJECT_SOURCE_DIR}/../img/${BDJ4INSTICON} .
    COMMAND echo "id ICON ${BDJ4INSTICON}" > selauncher.rc
    COMMAND windres selauncher.rc -O coff -o selauncher.o
    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/../img/${BDJ4INSTICON}
    BYPRODUCTS selauncher.rc ${BDJ4INSTICON}
    VERBATIM
  )
endif()

add_executable (bdj4se bdj4se.c)
if (WIN32)
  target_compile_options (bdj4se PRIVATE -municode)
endif()
target_link_libraries (bdj4se
  libbdj4fileop
  libbdj4string
)
if (WIN32)
  target_sources (bdj4se PRIVATE selauncher.o)
endif()

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
endif()
if (WIN32)
  target_sources (bdj4 PRIVATE launcher.o)
  target_compile_options (bdj4 PRIVATE -mwindows)
endif()
target_link_libraries (bdj4
  libbdj4launcher
  libbdj4fileop
  libbdj4string
)
# for gethostname
addWinSockLibrary (bdj4)
addWinNtdllLibrary (bdj4)

# this is only used on macos
add_executable (bdj4g bdj4.c)
if (WIN32)
  target_compile_options (bdj4g PRIVATE -municode)
endif()
target_include_directories (bdj4g
  PUBLIC ${GTK_INCLUDE_DIRS}
)
if (WIN32)
  target_compile_options (bdj4g PRIVATE -mwindows)
endif()
target_compile_options (bdj4g PRIVATE -DBDJ4_GUI_LAUNCHER=1)
target_link_libraries (bdj4g
  libbdj4launcher
  libbdj4fileop
  libbdj4string
  ${GTK_LDFLAGS}
)
addWinSockLibrary (bdj4g)
addWinNtdllLibrary (bdj4g)

add_executable (bdj4dbupdate bdj4dbupdate.c)
if (WIN32)
  target_compile_options (bdj4dbupdate PRIVATE -municode)
endif()
if (WIN32)
  target_compile_options (bdj4dbupdate PRIVATE -mwindows)
endif()
target_link_libraries (bdj4dbupdate
  libbdjregex libbdj4 libbdj4common
  m
)
if (WIN32)
  target_link_libraries (bdj4dbupdate -lregex)
endif()
addIntlLibrary (bdj4dbupdate)
addWinSockLibrary (bdj4dbupdate)

add_executable (bdj4dbtag bdj4dbtag.c)
if (WIN32)
  target_compile_options (bdj4dbtag PRIVATE -municode)
endif()
target_link_libraries (bdj4dbtag
  libbdj4 libbdj4common
  m
)
if (WIN32)
  target_compile_options (bdj4dbtag PRIVATE -mwindows)
endif()
target_compile_options (bdj4dbtag PRIVATE -pthread)
target_link_options (bdj4dbtag PRIVATE -pthread)
addIntlLibrary (bdj4dbtag)
addWinSockLibrary (bdj4dbtag)

add_executable (bdj4main bdj4main.c)
if (WIN32)
  target_compile_options (bdj4main PRIVATE -municode)
endif()
target_link_libraries (bdj4main
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
if (WIN32)
  target_compile_options (bdj4main PRIVATE -mwindows)
endif()
addIntlLibrary (bdj4main)
addWinSockLibrary (bdj4main)

add_executable (bdj4player bdj4player.c)
if (WIN32)
  target_compile_options (bdj4player PRIVATE -municode)
endif()
target_link_libraries (bdj4player
  libbdj4pli libbdj4vol libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addIntlLibrary (bdj4player)
addWinSockLibrary (bdj4player)

add_executable(bdj4mobmq bdj4mobmq.c)
target_include_directories (bdj4mobmq
  PRIVATE "${MONGOOSE_INC_DIR}"
)
if (WIN32)
  target_compile_options (bdj4mobmq PRIVATE -municode)
endif()
if (WIN32)
  target_compile_options (bdj4mobmq PRIVATE -mwindows)
endif()
target_link_libraries (bdj4mobmq
  libwebsrv
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addWinSockLibrary (bdj4mobmq)

add_executable(bdj4rc bdj4rc.c)
target_include_directories (bdj4rc
  PRIVATE "${MONGOOSE_INC_DIR}"
)
if (WIN32)
  target_compile_options (bdj4rc PRIVATE -municode)
endif()
if (WIN32)
  target_compile_options (bdj4rc PRIVATE -mwindows)
endif()
target_link_libraries (bdj4rc
  libwebsrv
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addWinSockLibrary (bdj4rc)

add_executable (bdj4marquee bdj4marquee.c)
if (WIN32)
  target_compile_options (bdj4marquee PRIVATE -municode)
endif()
if (WIN32)
  target_compile_options (bdj4marquee PRIVATE -mwindows)
endif()
target_link_libraries (bdj4marquee
  libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4marquee)
addWinSockLibrary (bdj4marquee)

add_executable (bdj4installer bdj4installer.c)
if (WIN32)
  target_compile_options (bdj4installer PRIVATE -municode)
endif()
target_link_libraries (bdj4installer
  liblocatebdj3
  libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4installer)

add_executable (bdj4starterui bdj4starterui.c)
if (WIN32)
  target_compile_options (bdj4starterui PRIVATE -municode)
endif()
if (WIN32)
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
endif()
if (WIN32)
  target_compile_options (bdj4playerui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4playerui
  libbdj4uigtk libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4playerui)
addWinSockLibrary (bdj4playerui)

add_executable (bdj4manageui bdj4manageui.c)
if (WIN32)
  target_compile_options (bdj4manageui PRIVATE -municode)
endif()
if (WIN32)
  target_compile_options (bdj4manageui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4manageui
  libbdj4uigtk libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4manageui)
addWinSockLibrary (bdj4manageui)

add_executable (bdj4configui bdj4configui.c)
if (WIN32)
  target_compile_options (bdj4configui PRIVATE -municode)
endif()
if (WIN32)
  target_compile_options (bdj4configui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4configui
  libbdj4ui libuigtk3 libbdj4vol libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4configui)
addWinSockLibrary (bdj4configui)

add_subdirectory (check)

install (TARGETS
  libbdj4common libbdj4
  libuigtk3
  bdj4 bdj4g
  bdj4se bdj4installer bdj4updater
  bdj4locatebdj3 bdj4info bdj4tags
  bdj4dbupdate bdj4dbtag
  bdj4main bdj4player bdj4mobmq bdj4rc bdj4marquee
  bdj4starterui bdj4playerui bdj4manageui bdj4configui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4)
  macUpdateRPath (bdj4dbupdate)
  macUpdateRPath (bdj4g)
  macUpdateRPath (bdj4info)
  macUpdateRPath (bdj4installer)
  macUpdateRPath (bdj4locatebdj3)
  macUpdateRPath (bdj4main)
  macUpdateRPath (bdj4marquee)
  macUpdateRPath (bdj4mobmq)
  macUpdateRPath (bdj4player)
  macUpdateRPath (bdj4playerui)
  macUpdateRPath (bdj4rc)
  macUpdateRPath (bdj4tags)
  macUpdateRPath (bdj4updater)
  macUpdateRPath (check_all)
endif()

