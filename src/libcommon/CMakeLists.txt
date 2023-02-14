#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4common SHARED
  bdjmsg.c
  bdjvars.c
  callback.c
  colorutils.c
  conn.c
  dirop.c
  filedata.c
  filemanip.c
  fileshared.c
  log.c
  osdir.c
  oslocale.c
  osnetutils.c
  osrandom.c
  ossignal.c
  pathbld.c
  pathutil.c
  queue.c
  sock.c
  sockh.c
  sysvars.c
  vsencdec.c
)
target_include_directories (libbdj4common
  PUBLIC "${GLIB_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4common
  libbdj4fileop     # fileop.c
  libbdj4osutils    # osutils.c, osprocess.c
  libbdj4tmutil     # tmutil.c
  libbdj4string     # bdjstring, strlcat, strlcpy, mdebug
  ${GLIB_LDFLAGS}
  m
)
addWinSockLibrary (libbdj4common)
# for RtlGetVersion
addWinNtdllLibrary (libbdj4common)

install (TARGETS
  libbdj4common
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

