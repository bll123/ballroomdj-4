cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

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

install (TARGETS
  bdj4configui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4configui)
endif()
