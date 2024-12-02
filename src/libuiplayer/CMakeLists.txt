#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4uip SHARED
  uiextreq.c
  uiquickedit.c
)
target_include_directories (libbdj4uip
  PRIVATE ${GTK_INCLUDE_DIRS}
)
if (APPLE)
  target_link_options (libbdj4uip PRIVATE
    -framework Cocoa
  )
endif()
target_link_libraries (libbdj4uip PRIVATE
  libbdj4uii libbdj4uib ${BDJ4_UI_LIB}
  libbdj4audiosrc
  libbdj4 libbdj4basic libbdj4common
  ${GTK_LDFLAGS}
)
addIntlLibrary (libbdj4uip)
addWinSockLibrary (libbdj4uip)

install (TARGETS
  libbdj4uip
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4uip)
