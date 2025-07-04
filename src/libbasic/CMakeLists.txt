#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4basic SHARED
  asconf.c
  bdjopt.c
  datafile.c
  dirlist.c
  ilist.c
  istring.c
  list.c
  localeutil.c
  lock.c
  nlist.c
  procutil.c
  progstate.c
  rafile.c
  slist.c
  xmlparse.c
)
target_include_directories (libbdj4basic
  PRIVATE "${GLIB_INCLUDE_DIRS}"
  PRIVATE "${XML2_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4basic PRIVATE
  libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  ${GLIB_LDFLAGS}
  ${XML2_LDFLAGS}
)

install (TARGETS
  libbdj4basic
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4basic)
