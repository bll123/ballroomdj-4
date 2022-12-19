#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

# for the time being, these need gtk
# gtk will be removed from these files eventually.

add_library (libbdj4ui SHARED
  osuilinuxutils.c
  osuimacutils.m
  osuiwinutils.c
  uidance.c
  uiduallist.c
  uifavorite.c
  uigenre.c
  uilevel.c
  uimusicq.c
  uimusicqcommon.c
  uimusicqgtk.c
  uinbutil.c
  uiplayer.c
  uirating.c
  uireqext.c
  uiselectfile.c
  uisong.c
  uisongedit.c
  uisongeditgtk.c
  uisongfilter.c
  uisongsel.c
  uisongselcommon.c
  uisongselgtk.c
  uistatus.c
)
target_include_directories (libbdj4ui
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_link_libraries (libbdj4ui
  libuigtk3 libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (libbdj4ui)
addWinSockLibrary (libbdj4ui)

install (TARGETS
  libbdj4ui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
