#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
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

# want to keep the launcher at a minimum size and statically linked
# with no shared library dependencies.
add_executable (bdj4 bdj4.c)
if (WIN32)
  target_compile_options (bdj4 PRIVATE -municode)
  target_sources (bdj4 PRIVATE launcher.o)
endif()
target_link_libraries (bdj4 PRIVATE
  libbdj4launcher
  libbdj4fileop
  libbdj4osprocess    # include osutils,bdj4string
  libbdj4tmutil
)
# for gethostname
addWinSockLibrary (bdj4)
addWinNtdllLibrary (bdj4)
updateRPath (bdj4)

# this is only used on macos
add_executable (bdj4g bdj4.c)
if (WIN32)
  target_compile_options (bdj4g PRIVATE -municode)
endif()
target_include_directories (bdj4g
  PRIVATE ${GTK_INCLUDE_DIRS}
)
target_compile_options (bdj4g PRIVATE -DBDJ4_GUI_LAUNCHER=1)
target_link_libraries (bdj4g PRIVATE
  libbdj4launcher
  libbdj4fileop
  libbdj4osprocess    # include osutils,bdj4string
  ${GTK_LDFLAGS}
)
addWinSockLibrary (bdj4g)
addWinNtdllLibrary (bdj4g)
updateRPath (bdj4g)

add_executable (bdj4starterui
  bdj4starterui.c
  startutil.c
)
if (WIN32)
  target_compile_options (bdj4starterui PRIVATE -municode)
  target_compile_options (bdj4starterui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4starterui PRIVATE
  libbdj4ui ${BDJ4_UI_LIB} libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (bdj4starterui)
addWinSockLibrary (bdj4starterui)
updateRPath (bdj4starterui)

add_executable (bdj4helperui bdj4helperui.c)
if (WIN32)
  target_compile_options (bdj4helperui PRIVATE -municode)
  target_compile_options (bdj4helperui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4helperui PRIVATE
  libbdj4ui ${BDJ4_UI_LIB} libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (bdj4helperui)
addWinSockLibrary (bdj4helperui)
updateRPath (bdj4helperui)

add_executable (bdj4cleantmp bdj4cleantmp.c)
if (WIN32)
  target_compile_options (bdj4cleantmp PRIVATE -municode)
endif()
target_link_libraries (bdj4cleantmp PRIVATE
  libbdj4basic libbdj4common
)
updateRPath (bdj4cleantmp)

install (TARGETS
  bdj4 bdj4g
  bdj4starterui bdj4helperui bdj4cleantmp
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

