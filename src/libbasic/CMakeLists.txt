#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4basic SHARED
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
)
target_include_directories (libbdj4basic
  PUBLIC "${GLIB_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4basic PUBLIC
  libbdj4common
  ${ICUI18N_LDFLAGS}
  ${ICUUC_LDFLAGS}
  ${GLIB_LDFLAGS}
  m
)

install (TARGETS
  libbdj4basic
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libbdj4basic)
