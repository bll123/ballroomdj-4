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
endif()
if (WIN32)
  target_compile_options (bdj4manageui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4manageui
  libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
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
target_link_libraries (bdj4bpmcounter
  libbdj4ui libuigtk3 libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4bpmcounter)
addWinSockLibrary (bdj4bpmcounter)

add_executable (bdj4dbupdate bdj4dbupdate.c)
if (WIN32)
  target_compile_options (bdj4dbupdate PRIVATE -municode)
  target_compile_options (bdj4dbupdate PRIVATE -mwindows)
endif()
target_link_libraries (bdj4dbupdate
  libbdjregex libbdj4 libbdj4common
  m
)
if (WIN32)
  target_link_libraries (bdj4dbupdate -lregex)
endif()
addIntlLibrary (bdj4dbupdate)
addWinSockLibrary (bdj4dbupdate)

add_executable (bdj4dbtag bdj4dbtag.c)
if (WIN32)
  target_compile_options (bdj4dbtag PRIVATE -municode)
  target_compile_options (bdj4dbtag PRIVATE -mwindows)
endif()
target_link_libraries (bdj4dbtag
  libbdj4 libbdj4common
  m
)
target_compile_options (bdj4dbtag PRIVATE -pthread)
target_link_options (bdj4dbtag PRIVATE -pthread)
addIntlLibrary (bdj4dbtag)
addWinSockLibrary (bdj4dbtag)

install (TARGETS
  bdj4manageui bdj4bpmcounter bdj4dbupdate bdj4dbtag
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4manageui)
  macUpdateRPath (bdj4bpmcounter)
  macUpdateRPath (bdj4dbupdate)
  macUpdateRPath (bdj4dbtag)
endif()
