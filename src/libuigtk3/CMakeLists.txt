#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libuigtk3 SHARED
  uigtkbox.c
  uigtkbutton.c
  uigtkchgind.c
  uigtkclipboard.c
  uigtkdialog.c
  uigtkdragdrop.c
  uigtkentry.c
  uigtkevents.c
  uigtkimage.c
  uigtklabel.c
  uigtklink.c
  uigtkmenu.c
  uigtkmiscbutton.c
  uigtknotebook.c
  uigtkpanedwindow.c
  uigtkpbar.c
  uigtkscale.c
  uigtkscrollbar.c
  uigtksep.c
  uigtksizegrp.c
  uigtkspinbox.c
  uigtkswitch.c
  uigtktextbox.c
  uigtktoggle.c
  uigtkui.c
  uigtkwidget.c
  uigtkwindow.c
)
target_include_directories (libuigtk3
  PRIVATE ${GTK_INCLUDE_DIRS}
)
target_link_libraries (libuigtk3 PRIVATE
  objuiwcont libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (libuigtk3)
addWinSockLibrary (libuigtk3)

install (TARGETS
  libuigtk3
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libuigtk3)
