cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4configui
  bdj4configui.c
  confcommon.c
  confdance.c
  confdebug.c
  confdispset.c
  conffilter.c
  confgeneral.c
  confgenre.c
  confgui.c
  confitunes.c
  conflevel.c
  confmarquee.c
  confmobmq.c
  conforg.c
  confplayer.c
  confrating.c
  confrc.c
  confstatus.c
  conftable.c
  conftableadd.c
  conftabledance.c
  conftableset.c
  confui.c
)
if (WIN32)
  target_compile_options (bdj4configui PRIVATE -municode)
  target_compile_options (bdj4configui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4configui
  libbdj4ui libuigtk3 libbdj4vol libbdj4pli libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4configui)
addWinSockLibrary (bdj4configui)

install (TARGETS
  bdj4configui
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4configui)
  macFixlibicu (bdj4configui)
endif()
