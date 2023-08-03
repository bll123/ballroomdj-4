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
  uiapplyadj.c
  uicopytags.c
  uidance.c
  uiduallist.c
  uiexpimpbdj4.c
  uifavorite.c
  uigenre.c
  uilevel.c
  uimusicq.c
  uimusicqcommon.c
  uimusicqui.c
  uinbutil.c
  uiplayer.c
  uiplaylist.c
  uirating.c
  uiextreq.c
  uiselectfile.c
  uisong.c
  uisongedit.c
  uisongeditui.c
  uisongfilter.c
  uisongsel.c
  uisongselcommon.c
  uisongselui.c
  uistatus.c
  uitreedisp.c
  uiutils.c
)
target_include_directories (libbdj4ui
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_link_libraries (libbdj4ui PUBLIC
  ${BDJ4_UI_LIB} libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  ${GTK_LDFLAGS}
  z
  m
)
addIntlLibrary (libbdj4ui)
addWinSockLibrary (libbdj4ui)

install (TARGETS
  libbdj4ui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libbdj4ui)
