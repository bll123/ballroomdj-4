#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (${BDJ4_UI_LIB} SHARED
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
addUIInclude (${BDJ4_UI_LIB})
target_link_libraries (${BDJ4_UI_LIB} PRIVATE
  objuigeneral objuios libbdj4basic libbdj4common
)
addUILibrary (${BDJ4_UI_LIB})
addIntlLibrary (${BDJ4_UI_LIB})
addWinSockLibrary (${BDJ4_UI_LIB})

uilibPreSymlink ()

install (TARGETS
  ${BDJ4_UI_LIB}
  DESTINATION ${DEST_BIN}
)

updateRPath (${BDJ4_UI_LIB})
uilibSymlink ()
