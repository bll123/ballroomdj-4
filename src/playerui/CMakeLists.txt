#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4playerui bdj4playerui.c)
if (WIN32)
  target_compile_options (bdj4playerui PRIVATE -municode)
  target_compile_options (bdj4playerui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4playerui PRIVATE
  libbdj4uip libbdj4uic libbdj4uii libbdj4uib ${BDJ4_UI_LIB} libbdj4cont
  libbdj4audiosrc
  libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4playerui)
addWinSockLibrary (bdj4playerui)
updateRPath (bdj4playerui)

install (TARGETS
  bdj4playerui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

