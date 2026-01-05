#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libwebclient SHARED
  webclient.c
)
target_include_directories (libwebclient
  PRIVATE "${PKG_GLIB_INCLUDE_DIRS}"
  PRIVATE "${PKG_CURL_INCLUDE_DIRS}"
  PRIVATE "${PKG_OPENSSL_INCLUDE_DIRS}"
)
target_link_libraries (libwebclient PRIVATE
  libbdj4basic libbdj4common
  ${PKG_GLIB_LDFLAGS}
  ${PKG_CURL_LDFLAGS} ${PKG_OPENSSL_LDFLAGS}
  z
)

install (TARGETS
  libwebclient
  DESTINATION ${DEST_BIN}
)

updateRPath (libwebclient)
