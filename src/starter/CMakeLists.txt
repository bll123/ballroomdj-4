cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

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
  libbdj4ui
  libuigtk3
  libbdj4
  libbdj4basic
  libbdj4common
  ${ICU_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  z
  m
)
addIntlLibrary (bdj4starterui)
addWinSockLibrary (bdj4starterui)

add_executable (bdj4helperui bdj4helperui.c)
if (WIN32)
  target_compile_options (bdj4helperui PRIVATE -municode)
  target_compile_options (bdj4helperui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4helperui
  libbdj4ui
  libuigtk3
  libbdj4
  libbdj4basic
  libbdj4common
  ${ICU_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4helperui)
addWinSockLibrary (bdj4helperui)

install (TARGETS
  bdj4 bdj4g
  bdj4starterui bdj4helperui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4)
  macFixlibicu (bdj4)
  macUpdateRPath (bdj4g)
  macFixlibicu (bdj4g)
  macUpdateRPath (bdj4starterui)
  macFixlibicu (bdj4starterui)
  macUpdateRPath (bdj4helperui)
  macFixlibicu (bdj4helperui)
endif()
