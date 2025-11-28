#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libuincurses SHARED
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
addUIInclude (libuincurses)
target_link_libraries (libuincurses PRIVATE
  objuigeneral libbdj4basic libbdj4common
)
addUILibrary (libuincurses)
addIntlLibrary (libuincurses)
addWinSockLibrary (libuincurses)

install (TARGETS
  libuincurses
  DESTINATION ${DEST_BIN}
)

updateRPath (libuincurses)
