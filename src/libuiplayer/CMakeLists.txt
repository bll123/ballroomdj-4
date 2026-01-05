#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4uip SHARED
  uiextreq.c
  uiquickedit.c
)
addUIInclude (libbdj4uip)
target_link_libraries (libbdj4uip PRIVATE
  libbdj4uii libbdj4uib ${BDJ4_UI_LIB}
  libbdj4audiosrc
  libbdj4 libbdj4basic libbdj4common
)
addUILibrary (libbdj4uip)
addIntlLibrary (libbdj4uip)
addWinSockLibrary (libbdj4uip)

install (TARGETS
  libbdj4uip
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uip)
