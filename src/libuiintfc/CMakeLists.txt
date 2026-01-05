#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4uii SHARED
  uidance.c
  uidd.c
  uiduallist.c
  uifavorite.c
  uigenre.c
  uilevel.c
  uinbutil.c
  uiplaylist.c
  uirating.c
  uiselectfile.c
  uisong.c
  uistatus.c
  uivirtlist.c
  uivlutil.c
  uivnb.c
)
addUIInclude (libbdj4uii)
target_link_libraries (libbdj4uii PRIVATE
  libbdj4uib ${BDJ4_UI_LIB}
  libbdj4 libbdj4basic libbdj4common
)
addUILibrary (libbdj4uii)
addIntlLibrary (libbdj4uii)
addWinSockLibrary (libbdj4uii)

install (TARGETS
  libbdj4uii
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uii)
