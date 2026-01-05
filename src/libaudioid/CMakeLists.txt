#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4audioid SHARED
  audioid.c
  audioidutil.c
  audioidjson.c
  audioidxml.c
  musicbrainz.c
  acoustid.c
  acrcloud.c
)
target_include_directories (libbdj4audioid
  PRIVATE "${PKG_GLIB_INCLUDE_DIRS}"
  PRIVATE "${PKG_XML2_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4audioid PRIVATE
  libwebclient libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
  ${PKG_GLIB_LDFLAGS}
  ${PKG_GCRYPT_LDFLAGS}
  ${PKG_XML2_LDFLAGS}
  ${PKG_JSONC_LDFLAGS}
)
# if (APPLE)
#  target_link_libraries (libbdj4audioid PRIVATE
#    "-framework Cocoa"
#  )
#endif()
addIntlLibrary (libbdj4audioid)
addWinSockLibrary (libbdj4audioid)

install (TARGETS
  libbdj4audioid
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4audioid)
