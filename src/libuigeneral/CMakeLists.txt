#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

# the various main ui libraries (libuigtk3, libuimacos) will link this in
add_library (objuigeneral OBJECT
  uibox.c
  uibutton.c
  uientry.c
  uientryval.c
  uigrid.c
  uiheadline.c
  uilabel.c
  uiutils.c
  uiwcont.c
  uiwcontfree.c
)
addUIInclude (objuigeneral)
