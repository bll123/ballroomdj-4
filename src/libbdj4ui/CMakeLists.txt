cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

# for the time being, these need gtk
# gtk will be removed from these files eventually.

add_library (libbdj4ui SHARED
  osuilinuxutils.c
  osuimacutils.m
  osuiwinutils.c
  uiduallist.c
  uimusicq.c
  uimusicqgtk.c
  uiplayer.c
  uisongedit.c
  uisongeditgtk.c
  uisongsel.c
  uisongselfilter.c
  uisongselgtk.c
  uiutils.c
)
target_include_directories (libbdj4ui
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_link_libraries (libbdj4ui
  libuigtk3 libbdj4 libbdj4common
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (libbdj4ui)
addWinSockLibrary (libbdj4ui)

install (TARGETS
  libbdj4ui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
