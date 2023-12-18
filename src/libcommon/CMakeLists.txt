#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4common SHARED
  bdj4arg.c
  bdjmsg.c
  bdjregex.c
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
  oslinuxlocale.c
  oslocale.c
  osmaclocale.m
  oswinlocale.c
  osnetutils.c
  osrandom.c
  ossignal.c
  pathbld.c
  pathdisp.c
  pathutil.c
  queue.c
  sock.c
  sockh.c
  sysvars.c
  vsencdec.c
)
target_include_directories (libbdj4common
  PRIVATE "${GLIB_INCLUDE_DIRS}"
)
if (APPLE)
  target_link_libraries (libbdj4common PRIVATE
    "-framework Cocoa"
  )
endif()
target_link_libraries (libbdj4common PRIVATE
  libbdj4fileop     # fileop.c
  libbdj4osprocess  # osprocess.c
  libbdj4tmutil     # tmutil.c
  libbdj4osutils    # osutils (bdjstring, strlcat, strlcpy, mdebug)
  ${GLIB_LDFLAGS}
)
addIOKitFramework (libbdj4common)
addIntlLibrary (libbdj4common)
addWinSockLibrary (libbdj4common)
# for RtlGetVersion
addWinNtdllLibrary (libbdj4common)

install (TARGETS
  libbdj4common
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libbdj4common)
