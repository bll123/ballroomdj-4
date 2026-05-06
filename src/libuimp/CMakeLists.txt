#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4uimp SHARED
  continst.c
  uimusicq.c
  uimusicqcommon.c
  uimusicqui.c
  uiplayer.c
  uisongfilter.c
  uisongsel.c
  uisongselcommon.c
  uisongselui.c
)
target_link_libraries (libbdj4uimp PRIVATE
  libbdj4uii libbdj4uig libbdj4ui libbdj4cont
  libbdj4audiosrc
  libbdj4 libbdj4basic libbdj4common
)
addIntlLibrary (libbdj4uimp)
addWinSockLibrary (libbdj4uimp)

install (TARGETS
  libbdj4uimp
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uimp)
