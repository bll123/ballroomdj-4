#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4uig SHARED
  uibox.c
  uibutton.c
  uientry.c
  uigrid.c
  uiheadline.c
  uilabel.c
  uiutils.c
  uiwcontfree.c
)
#addUIInclude (libbdj4uig)
target_link_libraries (libbdj4uig PRIVATE
  libbdj4ui
  libbdj4 libbdj4basic libbdj4common
)
#addIntlLibrary (libbdj4uig)
install (TARGETS
  libbdj4uig
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uig)

# the various main ui libraries (libuigtk3, libuimacos) will link this in
add_library (objuigeneral
  uientryval.c
  uiwcont.c
)
