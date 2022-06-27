cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4bpmcounter
  bdj4bpmcounter.c
)
if (WIN32)
  target_compile_options (bdj4bpmcounter PRIVATE -municode)
  target_compile_options (bdj4bpmcounter PRIVATE -mwindows)
endif()
target_link_libraries (bdj4bpmcounter
  libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4bpmcounter)
addWinSockLibrary (bdj4bpmcounter)

install (TARGETS
  bdj4bpmcounter
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4bpmcounter)
endif()
