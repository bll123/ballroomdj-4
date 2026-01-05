#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libuinull SHARED
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
addUIInclude (libuinull)
target_link_libraries (libuinull PRIVATE
  objuigeneral libbdj4basic libbdj4common
)
addUILibrary (libuinull)
addIntlLibrary (libuinull)
addWinSockLibrary (libuinull)

install (TARGETS
  libuinull
  DESTINATION ${DEST_BIN}
)

updateRPath (libuinull)
