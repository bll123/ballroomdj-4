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
target_link_libraries (libbdj4uim PRIVATE
  libbdj4uimp libbdj4uii libbdj4uig libbdj4ui
  libbdj4audiosrc
  libbdj4 libbdj4basic libbdj4common
)
addIntlLibrary (libbdj4uim)
addWinSockLibrary (libbdj4uim)

install (TARGETS
  libbdj4uim
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uim)
