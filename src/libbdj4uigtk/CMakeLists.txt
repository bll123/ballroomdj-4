cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4uigtk SHARED
  uiplayer.c
  uisongsel.c
  uimusicq.c
  uimusicqgtk.c
  uisongselgtk.c
)
target_include_directories (libbdj4uigtk
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_link_libraries (libbdj4uigtk
  libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (libbdj4uigtk)
addWinSockLibrary (libbdj4uigtk)

install (TARGETS
  libbdj4uigtk
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
