#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libuigtk3 SHARED
  uibox.c
  uibutton.c
  uichgind.c
  uiclipboard.c
  uidialog.c
  uidragdrop.c
  uientry.c
  uievents.c
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
  uisizegrp.c
  uispinbox.c
  uiswitch.c
  uitextbox.c
  uitoggle.c
  uiui.c
  uiwidget.c
  uiwindow.c
)
target_include_directories (libuigtk3
  PRIVATE ${GTK_INCLUDE_DIRS}
)
target_link_libraries (libuigtk3 PRIVATE
  objuigeneral libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (libuigtk3)
addWinSockLibrary (libuigtk3)

install (TARGETS
  libuigtk3
  DESTINATION ${DEST_BIN}
)

updateRPath (libuigtk3)
