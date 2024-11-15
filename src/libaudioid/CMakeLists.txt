#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
  PRIVATE "${GLIB_INCLUDE_DIRS}"
  PRIVATE "${XML2_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4audioid PRIVATE
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
  ${CURL_LDFLAGS}
  ${GLIB_LDFLAGS}
  ${GCRYPT_LDFLAGS}
  ${XML2_LDFLAGS}
  ${JSONC_LDFLAGS}
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
