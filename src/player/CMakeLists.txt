#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4main bdj4main.c)
if (WIN32)
  target_compile_options (bdj4main PRIVATE -municode)
endif()
target_link_libraries (bdj4main PRIVATE
  libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
)
addIntlLibrary (bdj4main)
addWinSockLibrary (bdj4main)
addWinVersionInfo (bdj4main)
addWinManifest (bdj4main)

add_executable (bdj4player bdj4player.c)
# threads are working 2025-7-30
target_compile_options (bdj4player PRIVATE -pthread)
if (WIN32)
  target_compile_options (bdj4player PRIVATE -municode)
endif()
target_link_libraries (bdj4player PRIVATE
  libbdj4pli libbdj4vol
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  pthread
  m
)
addIntlLibrary (bdj4player)
addWinSockLibrary (bdj4player)
addWinVersionInfo (bdj4player)
addWinManifest (bdj4player)


add_executable (bdj4marquee bdj4marquee.c)
if (WIN32)
  target_compile_options (bdj4marquee PRIVATE -municode)
  target_compile_options (bdj4marquee PRIVATE -mwindows)
endif()
target_link_libraries (bdj4marquee PRIVATE
  libbdj4uib ${BDJ4_UI_LIB}
  libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4marquee)
addWinSockLibrary (bdj4marquee)
addWinVersionInfo (bdj4marquee)
addWinManifest (bdj4marquee)

add_executable(bdj4rc bdj4rc.c)
target_include_directories (bdj4rc
  PRIVATE "${MONGOOSE_INC_DIR}"
)
if (WIN32)
  target_compile_options (bdj4rc PRIVATE -municode)
endif()
target_link_libraries (bdj4rc PRIVATE
  objwebsrv
  libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
)
addWinSockLibrary (bdj4rc)
addWinVersionInfo (bdj4rc)
addWinManifest (bdj4rc)

add_executable(bdj4mobmq bdj4mobmq.c)
target_include_directories (bdj4mobmq
  PRIVATE "${MONGOOSE_INC_DIR}"
)
if (WIN32)
  target_compile_options (bdj4mobmq PRIVATE -municode)
endif()
target_link_libraries (bdj4mobmq PRIVATE
  objwebsrv
  libwebclient libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
)
addIntlLibrary (bdj4mobmq)
addWinSockLibrary (bdj4mobmq)
addWinVersionInfo (bdj4mobmq)
addWinManifest (bdj4mobmq)

install (TARGETS
  bdj4main bdj4player bdj4marquee bdj4rc bdj4mobmq
  DESTINATION ${DEST_BIN}
)

updateRPath (bdj4main)
updateRPath (bdj4player)
updateRPath (bdj4marquee)
updateRPath (bdj4rc)
updateRPath (bdj4mobmq)
