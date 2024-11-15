#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

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
target_link_libraries (bdj4se PRIVATE
  objdirutil
  objosprocess
  objosenv
  objfileop
  objosutils    # needed for windows
  objstring
  objtmutil
  m
)
if (WIN32)
  target_sources (bdj4se PRIVATE selauncher.o)
endif()
addIOKitFramework (bdj4se)
addIntlLibrary (bdj4se)

add_executable (bdj4installer bdj4installer.c locatebdj3.c)
if (WIN32)
  target_compile_options (bdj4installer PRIVATE -municode)
  target_compile_options (bdj4installer PRIVATE -mwindows)
endif()
target_link_libraries (bdj4installer PRIVATE
  libbdj4uib ${BDJ4_UI_LIB}
  libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (bdj4installer)

add_executable (bdj4altinst bdj4altinst.c)
if (WIN32)
  target_compile_options (bdj4altinst PRIVATE -municode)
  target_compile_options (bdj4altinst PRIVATE -mwindows)
endif()
target_link_libraries (bdj4altinst PRIVATE
  libbdj4uib ${BDJ4_UI_LIB}
  libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (bdj4altinst)

add_executable (bdj4updater bdj4updater.c)
if (WIN32)
  target_compile_options (bdj4updater PRIVATE -municode)
endif()
target_link_libraries (bdj4updater PRIVATE
  libbdj4ati
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)
addIntlLibrary (bdj4updater)

install (TARGETS
  bdj4se bdj4installer bdj4altinst bdj4updater
  DESTINATION ${DEST_BIN}
)

updateRPath (bdj4installer)
updateRPath (bdj4altinst)
updateRPath (bdj4updater)
