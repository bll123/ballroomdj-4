#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libuinull SHARED
  uinulladjustment.c
  uinullbox.c
  uinullbutton.c
  uinullchgind.c
  uinullclipboard.c
  uinulldialog.c
  uinulldragdrop.c
  uinulldropdown.c
  uinullentry.c
  uinullimage.c
  uinullkeys.c
  uinulllabel.c
  uinulllink.c
  uinullmenu.c
  uinullmiscbutton.c
  uinullnotebook.c
  uinullpbar.c
  uinullscale.c
  uinullscrollbar.c
  uinullsep.c
  uinullsizegrp.c
  uinullspinbox.c
  uinullswitch.c
  uinulltextbox.c
  uinulltoggle.c
  uinulltreeview.c
  uinullui.c
  uinullwidget.c
  uinullwindow.c
  uiwcont.c
)
target_link_libraries (libuinull PUBLIC
  libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  z
  m
)
addIntlLibrary (libuinull)
addWinSockLibrary (libuinull)

install (TARGETS
  libuinull
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libuinull)
