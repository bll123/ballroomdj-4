cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libuigtk3 SHARED
  uigeneral.c
  uigtkbox.c
  uigtkbutton.c
  uigtkchgind.c
  uigtkdialog.c
  uigtkdropdown.c
  uigtkentry.c
  uigtkimage.c
  uigtklabel.c
  uigtklink.c
  uigtkmenu.c
  uigtknotebook.c
  uigtkpbar.c
  uigtkscale.c
  uigtksep.c
  uigtksizegrp.c
  uigtkspinbox.c
  uigtkswitch.c
  uigtktextbox.c
  uigtktoggle.c
  uigtktreeview.c
  uigtkutils.c
  uigtkwidget.c
  uigtkwindow.c
)
target_include_directories (libuigtk3
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_link_libraries (libuigtk3
  libbdj4 libbdj4common
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (libuigtk3)
addWinSockLibrary (libuigtk3)

install (TARGETS
  libuigtk3
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
