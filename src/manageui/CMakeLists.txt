#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4manageui
  bdj4manageui.c
  manageaudioid.c
  managedb.c
  managemisc.c
  managepl.c
  managepldance.c
  manageseq.c
  managestats.c
)
if (WIN32)
  target_compile_options (bdj4manageui PRIVATE -municode)
  target_compile_options (bdj4manageui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4manageui PRIVATE
  libbdj4uim libbdj4uic libbdj4uii libbdj4uib ${BDJ4_UI_LIB}
  libbdj4cont libbdj4audioid
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (bdj4manageui)
addWinSockLibrary (bdj4manageui)
addWinVersionInfo (bdj4manageui)
addWinManifest (bdj4manageui)

add_executable (bdj4bpmcounter
  bdj4bpmcounter.c
)
if (WIN32)
  target_compile_options (bdj4bpmcounter PRIVATE -municode)
  target_compile_options (bdj4bpmcounter PRIVATE -mwindows)
endif()
target_link_libraries (bdj4bpmcounter PRIVATE
  libbdj4uib ${BDJ4_UI_LIB} libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4bpmcounter)
addWinSockLibrary (bdj4bpmcounter)
addWinVersionInfo (bdj4bpmcounter)
addWinManifest (bdj4bpmcounter)

add_executable (bdj4dbupdate bdj4dbupdate.c)
if (WIN32)
  target_compile_options (bdj4dbupdate PRIVATE -municode)
endif()
target_link_libraries (bdj4dbupdate PRIVATE
  libbdj4ati
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
)
addIntlLibrary (bdj4dbupdate)
addWinSockLibrary (bdj4dbupdate)
addWinVersionInfo (bdj4dbupdate)
addWinManifest (bdj4dbupdate)

add_executable (bdj4podcastupd bdj4podcastupd.c)
if (WIN32)
  target_compile_options (bdj4podcastupd PRIVATE -municode)
endif()
target_link_libraries (bdj4podcastupd PRIVATE
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
)
addWinVersionInfo (bdj4podcastupd)
addWinManifest (bdj4podcastupd)

install (TARGETS
  bdj4manageui bdj4bpmcounter bdj4dbupdate bdj4podcastupd
  DESTINATION ${DEST_BIN}
)

updateRPath (bdj4manageui)
updateRPath (bdj4bpmcounter)
updateRPath (bdj4dbupdate)
updateRPath (bdj4podcastupd)
