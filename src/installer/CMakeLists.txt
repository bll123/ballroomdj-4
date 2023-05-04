#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
target_link_libraries (bdj4se PUBLIC
  libbdj4fileop
  libbdj4osutils
  libbdj4string
  m
)
if (WIN32)
  target_sources (bdj4se PRIVATE selauncher.o)
endif()

add_executable (bdj4installer bdj4installer.c)
if (WIN32)
  target_compile_options (bdj4installer PRIVATE -municode)
endif()
target_link_libraries (bdj4installer PUBLIC
  liblocatebdj3
  libbdj4ui ${BDJ4_UI_LIB} libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  z
  m
)
addIntlLibrary (bdj4installer)

add_executable (bdj4altsetup bdj4altsetup.c)
if (WIN32)
  target_compile_options (bdj4altsetup PRIVATE -municode)
endif()
target_link_libraries (bdj4altsetup PUBLIC
  liblocatebdj3
  libbdj4ui ${BDJ4_UI_LIB} libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  z
  m
)
addIntlLibrary (bdj4altsetup)

add_executable (bdj4updater bdj4updater.c)
if (WIN32)
  target_compile_options (bdj4updater PRIVATE -municode)
  target_compile_options (bdj4updater PRIVATE -mwindows)
endif()
target_link_libraries (bdj4updater PUBLIC
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  z
  m
)
addIntlLibrary (bdj4updater)

install (TARGETS
  bdj4se bdj4installer bdj4altsetup bdj4updater
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (bdj4se)
updateRPath (bdj4installer)
updateRPath (bdj4altsetup)
updateRPath (bdj4updater)
