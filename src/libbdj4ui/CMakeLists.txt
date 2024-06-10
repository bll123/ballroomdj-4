#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
  uiaudioid.c
  uiaudioidui.c
  uicopytags.c
  uidance.c
  uiduallist.c
  uiexppl.c
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
  uiquickedit.c
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
  uivirtlist.c
)
target_include_directories (libbdj4ui
  PRIVATE ${GTK_INCLUDE_DIRS}
)
if (APPLE)
  target_link_libraries (libbdj4ui PRIVATE
    "-framework Cocoa"
  )
endif()
target_link_libraries (libbdj4ui PRIVATE
  ${BDJ4_UI_LIB} libbdj4ati
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (libbdj4ui)
addWinSockLibrary (libbdj4ui)

install (TARGETS
  libbdj4ui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libbdj4ui)
