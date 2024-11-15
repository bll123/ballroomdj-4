#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
)
target_include_directories (libbdj4uii
  PRIVATE ${GTK_INCLUDE_DIRS}
)
if (APPLE)
  target_link_options (libbdj4uii PRIVATE
    -framework Cocoa
  )
endif()
target_link_libraries (libbdj4uii PRIVATE
  libbdj4uib ${BDJ4_UI_LIB}
  libbdj4 libbdj4basic libbdj4common
  ${GTK_LDFLAGS}
)
addIntlLibrary (libbdj4uii)
addWinSockLibrary (libbdj4uii)

install (TARGETS
  libbdj4uii
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uii)
