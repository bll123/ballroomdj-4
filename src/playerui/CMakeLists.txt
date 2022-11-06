cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4playerui bdj4playerui.c)
if (WIN32)
  target_compile_options (bdj4playerui PRIVATE -municode)
  target_compile_options (bdj4playerui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4playerui
  libbdj4ui libuigtk3 libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4playerui)
addWinSockLibrary (bdj4playerui)


install (TARGETS
  bdj4playerui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4playerui)
endif()
