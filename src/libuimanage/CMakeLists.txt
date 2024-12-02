#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
  uisongedit.c
  uisongeditui.c
)
target_include_directories (libbdj4uim
  PRIVATE ${GTK_INCLUDE_DIRS}
)
if (APPLE)
  target_link_options (libbdj4uim PRIVATE
    -framework Cocoa
  )
endif()
target_link_libraries (libbdj4uim PRIVATE
  libbdj4uic libbdj4uii libbdj4uib ${BDJ4_UI_LIB}
  libbdj4audiosrc
  libbdj4 libbdj4basic libbdj4common
  ${GTK_LDFLAGS}
)
addIntlLibrary (libbdj4uim)
addWinSockLibrary (libbdj4uim)

install (TARGETS
  libbdj4uim
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uim)
