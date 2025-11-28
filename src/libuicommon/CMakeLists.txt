#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4uic SHARED
  uimusicq.c
  uimusicqcommon.c
  uimusicqui.c
  uiplayer.c
  uisongfilter.c
  uisongsel.c
  uisongselcommon.c
  uisongselui.c
)
target_include_directories (libbdj4uic
  PRIVATE ${PKG_GTK_INCLUDE_DIRS}
)
if (APPLE)
  target_link_options (libbdj4uic PRIVATE
    -framework Cocoa
  )
endif()
target_link_libraries (libbdj4uic PRIVATE
  libbdj4uii libbdj4uib ${BDJ4_UI_LIB} libbdj4cont
  libbdj4audiosrc
  libbdj4 libbdj4basic libbdj4common
  ${PKG_GTK_LDFLAGS}
)
addIntlLibrary (libbdj4uic)
addWinSockLibrary (libbdj4uic)

install (TARGETS
  libbdj4uic
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uic)
