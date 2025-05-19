#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libwebclient SHARED
  webclient.c
)
target_include_directories (libwebclient
  PRIVATE "${GLIB_INCLUDE_DIRS}"
  PRIVATE "${CURL_INCLUDE_DIRS}"
  PRIVATE "${OPENSSL_INCLUDE_DIRS}"
)
target_link_libraries (libwebclient PRIVATE
  libbdj4basic libbdj4common
  ${GLIB_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  z
)

install (TARGETS
  libwebclient
  DESTINATION ${DEST_BIN}
)

updateRPath (libwebclient)
