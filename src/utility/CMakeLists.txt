#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

# the executables starting with 't' are used for testing.
# the executables starting with 'd' are used for debug purposes.

add_executable (bdj4info bdj4info.c)
if (WIN32)
  target_compile_options (bdj4info PRIVATE -municode)
endif()
target_include_directories (bdj4info
  PRIVATE ${GTK_INCLUDE_DIRS}
)
target_link_libraries (bdj4info PRIVATE
  libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)
addIntlLibrary (bdj4info)

add_executable (bdj4tags bdj4tags.c)
if (WIN32)
  target_compile_options (bdj4tags PRIVATE -municode)
endif()
target_link_libraries (bdj4tags PRIVATE
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)
addIntlLibrary (bdj4tags)

add_executable (bdj4winmksc bdj4winmksc.cpp)
if (WIN32)
  target_compile_options (bdj4winmksc PRIVATE -municode)
endif()

add_executable (dmkmfromdb dmkmfromdb.c)
if (WIN32)
  target_compile_options (dmkmfromdb PRIVATE -municode)
endif()
target_link_libraries (dmkmfromdb PRIVATE
  libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)
addIntlLibrary (dmkmfromdb)

add_executable (tdbcompare tdbcompare.c)
if (WIN32)
  target_compile_options (tdbcompare PRIVATE -municode)
endif()
target_link_libraries (tdbcompare PRIVATE
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)

add_executable (tdbsetval tdbsetval.c)
if (WIN32)
  target_compile_options (tdbsetval PRIVATE -municode)
endif()
target_link_libraries (tdbsetval PRIVATE
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)

add_executable (testsuite
  testsuite.c
)
target_link_libraries (testsuite PRIVATE
  libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)

add_executable (tmusicsetup tmusicsetup.c)
if (WIN32)
  target_compile_options (tmusicsetup PRIVATE -municode)
endif()
target_link_libraries (tmusicsetup PRIVATE
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)
addIntlLibrary (tmusicsetup)

add_executable (ttagdbchk ttagdbchk.c)
if (WIN32)
  target_compile_options (ttagdbchk PRIVATE -municode)
endif()
target_link_libraries (ttagdbchk PRIVATE
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)

add_executable (plisinklist plisinklist.c)
target_link_libraries (plisinklist PRIVATE
  libbdj4pli libbdj4vol libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
)

add_executable (dbustest dbustest.c)
target_link_libraries (dbustest PRIVATE
  objdbusi libbdj4common
)

add_executable (aesed aesed.c)
target_link_libraries (aesed PRIVATE
  libbdj4 libbdj4basic libbdj4common
  ${GCRYPT_LDFLAGS}
  ${GLIB_LDFLAGS}
)

add_executable (vsed vsed.c)
target_link_libraries (vsed PRIVATE
  libbdj4common
)

add_executable (uitest
  uitest.c
)
if (WIN32)
  target_compile_options (uitest PRIVATE -municode)
  target_compile_options (uitest PRIVATE -mwindows)
endif()
target_link_libraries (uitest PRIVATE
  libbdj4ui ${BDJ4_UI_LIB}
  libbdj4 libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${GTK_LDFLAGS}
)
addIntlLibrary (uitest)

if (LIBVLC_FOUND)
  add_executable (vlcversion
    vlcversion.c
  )
  if (WIN32)
    target_compile_options (uitest PRIVATE -municode)
    target_compile_options (uitest PRIVATE -mwindows)
  endif()
  target_include_directories (vlcversion
    PRIVATE "${LIBVLC_INCLUDE_DIR}"
  )
  target_link_libraries (vlcversion PRIVATE
    ${LIBVLC_LIBRARY}
  )
endif()

install (TARGETS
  bdj4info
  bdj4tags
  bdj4winmksc
  dmkmfromdb
  tdbcompare
  tdbsetval
  testsuite
  tmusicsetup
  ttagdbchk
  plisinklist
  dbustest
  aesed
  vsed
  uitest
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
if (LIBVLC_FOUND)
  install (TARGETS
    vlcversion
    DESTINATION ${PROJECT_SOURCE_DIR}/../bin
  )
endif()

updateRPath (bdj4info)
updateRPath (bdj4tags)
updateRPath (bdj4winmksc)
updateRPath (dmkmfromdb)
updateRPath (tdbcompare)
updateRPath (tdbsetval)
updateRPath (testsuite)
updateRPath (tmusicsetup)
updateRPath (ttagdbchk)
updateRPath (dbustest)
updateRPath (plisinklist)
updateRPath (vsed)
updateRPath (aesed)
updateRPath (uitest)
if (LIBVLC_FOUND)
  updateRPath (vlcversion)
endif()
