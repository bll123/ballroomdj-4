#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4ui SHARED
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
target_compile_options (libbdj4ui PRIVATE
  -DBDJ4_UI_MACOS=1
)
target_link_libraries (libbdj4ui PRIVATE
  objuigeneral objuios libbdj4basic libbdj4common
)
target_link_options (libbdj4ui PRIVATE
  -framework Cocoa
)
addIntlLibrary (libbdj4ui)
addWinSockLibrary (libbdj4ui)

uilibPreSymlink (libuimacos)

install (TARGETS
  libbdj4ui
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4ui)

uilibSymlink (libbdj4ui libuimacos)
