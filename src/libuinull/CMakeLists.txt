#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (objuiinit OBJECT
  uiinit.c
)
target_compile_options (objuiinit PRIVATE
  -DBDJ4_UI_GTK3=1
)
target_include_directories (objuiinit PRIVATE
  ${PKG_GTK_INCLUDE_DIRS}
)
target_link_libraries (objuiinit PRIVATE ${PKG_GTK_LDFLAGS})

add_library (libbdj4ui SHARED
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
  uiswitch.c
  uitextbox.c
  uitoggle.c
  uiui.c
  uiwidget.c
  uiwindow.c
)
target_compile_options (libbdj4ui PRIVATE
  -DBDJ4_UI_NULL=1
)
addUIInclude (libbdj4ui)
target_link_libraries (libbdj4ui PRIVATE
  objuigeneral objuios objuiinit libbdj4basic libbdj4common
)
addUILibrary (libbdj4ui)
addIntlLibrary (libbdj4ui)
addWinSockLibrary (libbdj4ui)

uilibPreSymlink (libbdj4ui)

install (TARGETS
  libbdj4ui
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4ui)
uilibSymlink (libbdj4ui ${BDJ4_UI_LIB_NAME})
