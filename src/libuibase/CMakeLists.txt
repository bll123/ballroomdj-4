#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4uib SHARED
  osuilinuxutils.c
  osuimacutils.m
  osuiwinutils.c
  uiutils.c
)
target_include_directories (libbdj4uib
  PRIVATE ${GTK_INCLUDE_DIRS}
)
if (APPLE)
  target_link_options (libbdj4uib PRIVATE
    -framework Cocoa
  )
endif()
target_link_libraries (libbdj4uib PRIVATE
  ${BDJ4_UI_LIB}
  libbdj4 libbdj4basic libbdj4common
  ${GTK_LDFLAGS}
)
addIntlLibrary (libbdj4uib)
addWinSockLibrary (libbdj4uib)

install (TARGETS
  libbdj4uib
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libbdj4uib)
