#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4uib SHARED
  uiutils.c
)
if (SYSMACOS)
  target_sources (libbdj4uib PRIVATE osuimacutils.m)
endif()
if (SYSWIN)
  target_sources (libbdj4uib PRIVATE osuiwinutils.c)
endif()
if (SYSLINUX)
  target_sources (libbdj4uib PRIVATE osuilinuxutils.c)
endif()
addUIInclude (libbdj4uib)
target_link_libraries (libbdj4uib PRIVATE
  ${BDJ4_UI_LIB}
  libbdj4 libbdj4basic libbdj4common
)
addUILibrary (libbdj4uib)
addIntlLibrary (libbdj4uib)
addWinSockLibrary (libbdj4uib)

install (TARGETS
  libbdj4uib
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uib)
