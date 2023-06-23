#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4 SHARED
  audioadjust.c
  audiotag.c
  autosel.c
  bdj4init.c
  bdjvarsdf.c
  bdjvarsdfload.c
  dance.c
  dancesel.c
  dispsel.c
  dnctypes.c
  expimpbdj4.c
  genre.c
  instutil.c
  itunes.c
  level.c
  m3u.c
  mp3exp.c
  msgparse.c
  musicdb.c
  musicq.c
  orgopt.c
  orgutil.c
  playlist.c
  rating.c
  samesong.c
  sequence.c
  song.c
  songdb.c
  songfav.c
  songfilter.c
  songlist.c
  songlistutil.c
  songsel.c
  songutil.c
  sortopt.c
  status.c
  support.c
  tagdef.c
  templateutil.c
  validate.c
  volreg.c
  webclient.c
)
target_include_directories (libbdj4
  PUBLIC "${CURL_INCLUDE_DIRS}"
  PUBLIC "${XML2_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4 PUBLIC
  libbdj4ati libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GLIB_LDFLAGS}
  ${XML2_LDFLAGS}
  z
  m
)
if (APPLE)
  target_link_libraries (libbdj4 PUBLIC
    "-framework Cocoa"
  )
endif()
addIntlLibrary (libbdj4)
addWinSockLibrary (libbdj4)

install (TARGETS
  libbdj4
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

