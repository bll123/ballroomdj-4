#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libuimacos SHARED
  uibox.m
  uibutton.m
  uichgind.m
  uiclipboard.m
  uidialog.m
  uidragdrop.m
  uientry.m
  uievents.m
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
  uisizegrp.m
  uispinbox.m
  uiswitch.m
  uitextbox.m
  uitoggle.m
  uiui.m
  uiwidget.m
  uiwindow.m
)
target_link_libraries (libuimacos PRIVATE
  objuigeneral libbdj4basic libbdj4common
)
target_link_options (libuimacos PRIVATE
  -framework Cocoa
)
addIntlLibrary (libuimacos)
addWinSockLibrary (libuimacos)

install (TARGETS
  libuimacos
  DESTINATION ${DEST_BIN}
)

updateRPath (libuimacos)
