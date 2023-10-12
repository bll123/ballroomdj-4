#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4audioid SHARED
  audioid.c
  audioidxml.c
  musicbrainz.c
  acoustid.c
  acrcloud.c
)
target_include_directories (libbdj4audioid
  PUBLIC "${XML2_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4audioid PUBLIC
  libbdj4 libbdj4basic libbdj4common
  ${CURL_LDFLAGS}
  ${GLIB_LDFLAGS}
  ${GCRYPT_LDFLAGS}
  ${XML2_LDFLAGS}
  ${JSONC_LDFLAGS}
  z
  m
)
# if (APPLE)
#  target_link_libraries (libbdj4audioid PUBLIC
#    "-framework Cocoa"
#  )
#endif()
addIntlLibrary (libbdj4audioid)
addWinSockLibrary (libbdj4audioid)

install (TARGETS
  libbdj4audioid
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libbdj4audioid)
