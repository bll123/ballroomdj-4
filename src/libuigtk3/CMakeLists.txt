#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4ui SHARED
CMakeLists.txt
  uibox.c
  uibutton.c
  uichgind.c
  uiclipboard.c
  uidialog.c
  uidragdrop.c
  uientry.c
  uievents.c
  uigrid.c
  uiimage.c
  uilabel.c
  uilink.c
  uimenu.c
  uimiscbutton.c
  uinotebook.c
  uipanedwindow.c
  uipbar.c
  uiscale.c
  uiscrollbar.c
  uisep.c
  uispecific.c
  uiswitch.c
  uitextbox.c
  uitoggle.c
  uiui.c
  uiwidget.c
  uiwindow.c
)
target_compile_options (libbdj4ui PRIVATE
  -DBDJ4_UI_GTK3=1
)
target_include_directories (libbdj4ui PRIVATE
  ${PKG_GTK_INCLUDE_DIRS}
)
target_link_libraries (libbdj4ui PRIVATE
  objuigeneral objuios libbdj4basic libbdj4common
)
target_link_libraries (libbdj4ui PRIVATE ${PKG_GTK_LDFLAGS})

addIntlLibrary (libbdj4ui)
addWinSockLibrary (libbdj4ui)

uilibPreSymlink (libbdj4ui)

install (TARGETS
  libbdj4ui
  DESTINATION ${DEST_BIN}
)
updateRPath (libbdj4ui)

uilibSymlink (libbdj4ui ${BDJ4_UI_LIB_NAME})
