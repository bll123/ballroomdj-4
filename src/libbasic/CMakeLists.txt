#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4basic SHARED
  asconf.c
  bdjopt.c
  datafile.c
  dirlist.c
  dyintfc.c
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
# the sleep utilities are in libbasic due
# to linux needing bdjopt
if (SYSMACOS)
  target_sources (libbdj4basic PRIVATE
    osmacsleep.m
  )
  target_link_options (libbdj4basic PRIVATE
    -framework Cocoa
  )
endif()
if (SYSWIN)
  target_sources (libbdj4basic PRIVATE
    oswinsleep.c
  )
endif()
if (SYSLINUX)
  target_sources (libbdj4basic PRIVATE
    oslinuxsleep.c
  )
  target_link_libraries (libbdj4basic PRIVATE
    ${PKG_GIO_LDFLAGS}
  )
endif()
target_include_directories (libbdj4basic
  PRIVATE "${PKG_XML2_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4basic PRIVATE
  libbdj4common
  ${PKG_XML2_LDFLAGS}
)
addIntlLibrary (libbdj4basic)
addIOKitFramework (libbdj4basic)

install (TARGETS
  libbdj4basic
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4basic)
