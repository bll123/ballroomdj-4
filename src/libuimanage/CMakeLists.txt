#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4uim SHARED
  uiapplyadj.c
  uiaudioid.c
  uiaudioidui.c
  uicopytags.c
  uiexpimpbdj4.c
  uiexppl.c
  uiimppl.c
  uisongedit.c
  uisongeditui.c
)
addUIInclude (libbdj4uim)
target_link_libraries (libbdj4uim PRIVATE
  libbdj4uic libbdj4uii libbdj4uib ${BDJ4_UI_LIB}
  libbdj4audiosrc
  libbdj4 libbdj4basic libbdj4common
)
addUILibrary (libbdj4uim)
addIntlLibrary (libbdj4uim)
addWinSockLibrary (libbdj4uim)

install (TARGETS
  libbdj4uim
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uim)
