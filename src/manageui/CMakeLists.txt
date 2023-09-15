#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4manageui
  bdj4manageui.c
  managedb.c
  managemisc.c
  managepl.c
  managepltree.c
  manageseq.c
  managestats.c
)
if (WIN32)
  target_compile_options (bdj4manageui PRIVATE -municode)
  target_compile_options (bdj4manageui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4manageui PUBLIC
  libbdj4ui ${BDJ4_UI_LIB} libbdj4audioid libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  z
  m
)
addIntlLibrary (bdj4manageui)
addWinSockLibrary (bdj4manageui)

add_executable (bdj4bpmcounter
  bdj4bpmcounter.c
)
if (WIN32)
  target_compile_options (bdj4bpmcounter PRIVATE -municode)
  target_compile_options (bdj4bpmcounter PRIVATE -mwindows)
endif()
target_link_libraries (bdj4bpmcounter PUBLIC
  libbdj4ui ${BDJ4_UI_LIB} libbdj4 libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  z
  m
)
addIntlLibrary (bdj4bpmcounter)
addWinSockLibrary (bdj4bpmcounter)

add_executable (bdj4dbupdate bdj4dbupdate.c)
if (WIN32)
  target_compile_options (bdj4dbupdate PRIVATE -municode)
endif()
target_link_libraries (bdj4dbupdate PUBLIC
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  z
  m
)
addIntlLibrary (bdj4dbupdate)
addWinSockLibrary (bdj4dbupdate)

add_executable (bdj4dbtag bdj4dbtag.c)
if (WIN32)
  target_compile_options (bdj4dbtag PRIVATE -municode)
endif()
target_link_libraries (bdj4dbtag PUBLIC
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS}
  z
  m
)
target_compile_options (bdj4dbtag PRIVATE -pthread)
target_link_options (bdj4dbtag PUBLIC -pthread)
addIntlLibrary (bdj4dbtag)
addWinSockLibrary (bdj4dbtag)

install (TARGETS
  bdj4manageui bdj4bpmcounter bdj4dbupdate bdj4dbtag
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (bdj4manageui)
updateRPath (bdj4bpmcounter)
updateRPath (bdj4dbupdate)
updateRPath (bdj4dbtag)
