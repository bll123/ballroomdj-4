#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4configui
  bdj4configui.c
  confcommon.c
  confdance.c
  confdebug.c
  confdispset.c
  confdialog.c
  confgeneral.c
  confgenre.c
  confgui.c
  confitunes.c
  conflevel.c
  confmarquee.c
  confmobmq.c
  confmusicq.c
  conforg.c
  confplayer.c
  confpopulate.c
  confrating.c
  confrc.c
  confstatus.c
  conftable.c
  conftableset.c
  confui.c
)
if (WIN32)
  target_compile_options (bdj4configui PRIVATE -municode)
  target_compile_options (bdj4configui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4configui PRIVATE
  libbdj4ui ${BDJ4_UI_LIB}
  libbdj4ati libbdj4pli libbdj4vol
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (bdj4configui)
addWinSockLibrary (bdj4configui)

install (TARGETS
  bdj4configui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (bdj4configui)
