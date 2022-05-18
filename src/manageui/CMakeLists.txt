cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4manageui
  bdj4manageui.c
  managemisc.c
  managepl.c
  manageseq.c
)
if (WIN32)
  target_compile_options (bdj4manageui PRIVATE -municode)
endif()
if (WIN32)
  target_compile_options (bdj4manageui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4manageui
  libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4manageui)
addWinSockLibrary (bdj4manageui)

install (TARGETS
  bdj4manageui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4manageui)
endif()
