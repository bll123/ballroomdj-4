cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4common SHARED
  bdj4reg.c
  bdjmsg.c
  bdjopt.c
  bdjstring.c
  bdjvars.c
  conn.c
  datafile.c
  dirop.c
  filedata.c
  filemanip.c
  fileop.c
  fileutil.c
  ilist.c
  list.c
  localeutil.c
  lock.c
  log.c
  nlist.c
  osnetutils.c
  osutils.c
  oslinuxutils.c
  osmacutils.m
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
