#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4 SHARED
  aesencdec.c
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
  grouping.c
  instutil.c
  itunes.c
  jspf.c
  level.c
  m3u.c
  mp3exp.c
  msgparse.c
  musicdb.c
  musicq.c
  orgopt.c
  orgutil.c
  playlist.c
  quickedit.c
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
  tagdef.c
  templateutil.c
  validate.c
  volreg.c
  webclient.c
  xspf.c
)
target_include_directories (libbdj4
  PRIVATE "${CURL_INCLUDE_DIRS}"
  PRIVATE "${GLIB_INCLUDE_DIRS}"
  PRIVATE "${XML2_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4 PRIVATE
  libbdj4ati
  libbdj4audiosrc libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GCRYPT_LDFLAGS}
  ${GLIB_LDFLAGS}
  ${XML2_LDFLAGS}
  ${JSONC_LDFLAGS}
  z
)
if (APPLE)
  target_link_options (libbdj4 PRIVATE
    -framework Cocoa
  )
endif()
addIntlLibrary (libbdj4)
addWinSockLibrary (libbdj4)

install (TARGETS
  libbdj4
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4)
