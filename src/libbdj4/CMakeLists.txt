cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4 SHARED
  audiotag.c
  autosel.c
  bdj4init.c
  bdjvarsdf.c
  bdjvarsdfload.c
  dance.c
  dancesel.c
  dispsel.c
  dnctypes.c
  genre.c
  level.c
  m3u.c
  musicdb.c
  musicq.c
  orgopt.c
  orgutil.c
  playlist.c
  rating.c
  sequence.c
  song.c
  songfav.c
  songfilter.c
  songlist.c
  songsel.c
  songutil.c
  sortopt.c
  status.c
  tagdef.c
  templateutil.c
  validate.c
  volreg.c
  webclient.c
)
target_include_directories (libbdj4
  PUBLIC "${CURL_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4
  libbdjregex libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GLIB_LDFLAGS}
  m
)
if (APPLE)
  target_link_libraries (libbdj4
    "-framework Cocoa"
  )
endif()
addIntlLibrary (libbdj4)
addWinSockLibrary (libbdj4)

install (TARGETS
  libbdj4
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
