#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable(bdj4server bdj4server.c)
target_include_directories (bdj4server
  PRIVATE "${MONGOOSE_INC_DIR}"
)
if (WIN32)
  target_compile_options (bdj4server PRIVATE -municode)
endif()
target_link_libraries (bdj4server PRIVATE
  libbdj4audiosrc
  libbdj4 libbdj4basic libbdj4common
  objwebsrv
  ${ICUI18N_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
)
addIntlLibrary (bdj4server)
addWinSockLibrary (bdj4server)
addWinVersionInfo (bdj4server)
addWinManifest (bdj4server)

install (TARGETS
  bdj4server
  DESTINATION ${DEST_BIN}
)

updateRPath (bdj4server)
