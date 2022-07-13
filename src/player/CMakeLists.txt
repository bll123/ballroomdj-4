cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4main bdj4main.c)
if (WIN32)
  target_compile_options (bdj4main PRIVATE -municode)
  target_compile_options (bdj4main PRIVATE -mwindows)
endif()
target_link_libraries (bdj4main
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addIntlLibrary (bdj4main)
addWinSockLibrary (bdj4main)

add_executable (bdj4player bdj4player.c)
if (WIN32)
  target_compile_options (bdj4player PRIVATE -municode)
  target_compile_options (bdj4player PRIVATE -mwindows)
endif()
target_link_libraries (bdj4player
  libbdj4pli libbdj4vol libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addIntlLibrary (bdj4player)
addWinSockLibrary (bdj4player)

add_executable (bdj4marquee bdj4marquee.c)
if (WIN32)
  target_compile_options (bdj4marquee PRIVATE -municode)
  target_compile_options (bdj4marquee PRIVATE -mwindows)
endif()
target_link_libraries (bdj4marquee
  libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4marquee)
addWinSockLibrary (bdj4marquee)

add_executable(bdj4rc bdj4rc.c)
target_include_directories (bdj4rc
  PRIVATE "${MONGOOSE_INC_DIR}"
)
if (WIN32)
  target_compile_options (bdj4rc PRIVATE -municode)
  target_compile_options (bdj4rc PRIVATE -mwindows)
endif()
target_link_libraries (bdj4rc
  libwebsrv
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addWinSockLibrary (bdj4rc)

add_executable(bdj4mobmq bdj4mobmq.c)
target_include_directories (bdj4mobmq
  PRIVATE "${MONGOOSE_INC_DIR}"
)
if (WIN32)
  target_compile_options (bdj4mobmq PRIVATE -municode)
  target_compile_options (bdj4mobmq PRIVATE -mwindows)
endif()
target_link_libraries (bdj4mobmq
  libwebsrv
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addWinSockLibrary (bdj4mobmq)

install (TARGETS
  bdj4main bdj4player bdj4marquee bdj4rc bdj4mobmq
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
 macUpdateRPath (bdj4main)
 macUpdateRPath (bdj4player)
 macUpdateRPath (bdj4marquee)
 macUpdateRPath (bdj4rc)
 macUpdateRPath (bdj4mobmq)
endif()
