cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4common SHARED
  bdjmsg.c
  bdjopt.c
  bdjstring.c
  bdjvars.c
  conn.c
  datafile.c
  dirlist.c
  dirop.c
  filedata.c
  filemanip.c
  fileop.c
  fileutil.c
  ilist.c
  istring.c
  list.c
  localeutil.c
  lock.c
  log.c
  misc.c
  nlist.c
  osdir.c
  oslocale.c
  osnetutils.c
  osprocess.c
  osrandom.c
  ossignal.c
  osutils.c
  oslinuxutils.c
  osmacutils.m
  oswinprocess.c
  oswinutils.c
  pathbld.c
  pathutil.c
  procutil.c
  progstate.c
  queue.c
  rafile.c
  slist.c
  sock.c
  sockh.c
  strlcat.c
  strlcpy.c
  sysvars.c
  tmutil.c
)
target_include_directories (libbdj4common
  PUBLIC "${GLIB_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4common
  ${GLIB_LDFLAGS}
)
addWinSockLibrary (libbdj4common)
# for RtlGetVersion
addWinNtdllLibrary (libbdj4common)

install (TARGETS
  libbdj4common
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
