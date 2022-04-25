cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4gtk SHARED
  uigtkbox.c
  uigtkbutton.c
  uigtkdialog.c
  uigtkdropdown.c
  uigtkentry.c
  uigtklabel.c
  uigtklink.c
  uigtknotebook.c
  uigtkpbar.c
  uigtkscale.c
  uigtkspinbox.c
  uigtktextbox.c
  uigtktreeview.c
  uigtkutils.c
  uigtkwindow.c
)
target_include_directories (libbdj4gtk
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_link_libraries (libbdj4gtk
  libbdj4 libbdj4common
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (libbdj4gtk)
addWinSockLibrary (libbdj4gtk)

install (TARGETS
  libbdj4gtk
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
