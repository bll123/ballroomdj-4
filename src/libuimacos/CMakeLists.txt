#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (${BDJ4_UI_LIB} SHARED
  uibox.m
  uibutton.m
  uichgind.m
  uiclipboard.m
  uidialog.m
  uidragdrop.m
  uientry.m
  uievents.m
  uigrid.m
  uiimage.m
  uilabel.m
  uilink.m
  uimenu.m
  uimiscbutton.m
  uinotebook.m
  uipanedwindow.m
  uipbar.m
  uiscale.m
  uiscrollbar.m
  uisep.m
  uiswitch.m
  uitextbox.m
  uitoggle.m
  uiui.m
  uiwidget.m
  uiwindow.m
)
target_link_libraries (${BDJ4_UI_LIB} PRIVATE
  objuigeneral libbdj4basic libbdj4common
)
target_link_options (${BDJ4_UI_LIB} PRIVATE
  -framework Cocoa
)
addIntlLibrary (${BDJ4_UI_LIB})
addWinSockLibrary (${BDJ4_UI_LIB})

uilibPreSymlink ()

install (TARGETS
  ${BDJ4_UI_LIB}
  DESTINATION ${DEST_BIN}
)

updateRPath (${BDJ4_UI_LIB})

uilibSymlink ()
