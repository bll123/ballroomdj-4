#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4configui
  bdj4configui.c
  confas.c
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
  confui.c
)
if (WIN32)
  target_compile_options (bdj4configui PRIVATE -municode)
  target_compile_options (bdj4configui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4configui PRIVATE
  libbdj4uii libbdj4uib ${BDJ4_UI_LIB}
  libbdj4ati libbdj4pli libbdj4vol libbdj4cont
  libwebclient libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (bdj4configui)
addWinSockLibrary (bdj4configui)
addWinVersionInfo (bdj4configui)
addWinManifest (bdj4configui)

install (TARGETS
  bdj4configui
  DESTINATION ${DEST_BIN}
)

updateRPath (bdj4configui)
