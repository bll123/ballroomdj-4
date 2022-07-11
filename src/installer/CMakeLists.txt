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
target_link_libraries (bdj4se
  libbdj4fileop
  libbdj4osutils
  libbdj4string
)
if (WIN32)
  target_sources (bdj4se PRIVATE selauncher.o)
endif()

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

add_executable (bdj4altinst bdj4altinst.c)
if (WIN32)
  target_compile_options (bdj4altinst PRIVATE -municode)
endif()
target_link_libraries (bdj4altinst
  liblocatebdj3
  libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4altinst)

add_executable (bdj4updater bdj4updater.c)
if (WIN32)
  target_compile_options (bdj4updater PRIVATE -municode)
  target_compile_options (bdj4updater PRIVATE -mwindows)
endif()
target_link_libraries (bdj4updater
  libbdj4common libbdj4
)
addIntlLibrary (bdj4updater)

install (TARGETS
  bdj4se bdj4installer bdj4altinst bdj4updater
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4se)
  macUpdateRPath (bdj4installer)
  macUpdateRPath (bdj4altinst)
  macUpdateRPath (bdj4updater)
endif()
