#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

# the executables starting with 't' are used for testing.
# the executables starting with 'd' are used for debug purposes.

add_executable (bdj4info bdj4info.c)
addUIInclude (bdj4info)
target_link_libraries (bdj4info PRIVATE
  libbdj4basic libbdj4common
)
addIntlLibrary (bdj4info)
addWinVersionInfo (bdj4info)
addWinManifest (bdj4info)

add_executable (bdj4tags bdj4tags.c)
target_link_libraries (bdj4tags PRIVATE
  libbdj4ati libbdj4 libbdj4basic libbdj4common
)
addIntlLibrary (bdj4tags)
addWinVersionInfo (bdj4tags)
addWinManifest (bdj4tags)

if (WIN32)
  add_executable (bdj4winmksc bdj4winmksc.cpp)
  addWinVersionInfo (bdj4winmksc)
  addWinManifest (bdj4winmksc)
endif()

if (WIN32)
  add_executable (bdj4winstartup bdj4winstartup.c)
  # this program should be set as a gui so that no command window pops up
  addWinGUIFlag (bdj4winstartup)
  target_link_options (bdj4winstartup PRIVATE -static-libgcc)
  target_link_libraries (bdj4winstartup PRIVATE
    objfileop
    objosenv
    objosprocess
    objosutils
    objpathutil
    objstring
    objtmutil
  )
  addWinVersionInfo (bdj4winstartup)
  addWinManifest (bdj4winstartup)
endif()

add_executable (dmkmfromdb dmkmfromdb.c)
target_link_libraries (dmkmfromdb PRIVATE
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
)
addIntlLibrary (dmkmfromdb)

add_executable (tdbcompare tdbcompare.c)
target_link_libraries (tdbcompare PRIVATE
  libbdj4ati libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
)

add_executable (tdbsetval tdbsetval.c)
target_link_libraries (tdbsetval PRIVATE
  libbdj4ati libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
)

add_executable (testsuite
  testsuite.c
)
target_link_libraries (testsuite PRIVATE
  libbdj4pli
  libbdj4 libbdj4basic libbdj4common
)

add_executable (tmusicsetup tmusicsetup.c)
target_link_libraries (tmusicsetup PRIVATE
  libbdj4ati libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
)
addIntlLibrary (tmusicsetup)

add_executable (ttagdbchk ttagdbchk.c)
target_link_libraries (ttagdbchk PRIVATE
  libbdj4ati libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
)

add_executable (plisinklist plisinklist.c)
target_link_libraries (plisinklist PRIVATE
  libbdj4pli libbdj4vol libbdj4basic libbdj4common
)

if (NOT WIN32 AND NOT APPLE)
  add_executable (dbustest dbustest.c)
  target_include_directories (dbustest
    PRIVATE "${PKG_GLIB_INCLUDE_DIRS}"
  )
  target_link_libraries (dbustest PRIVATE
    libbdj4common
    ${PKG_GIO_LDFLAGS}
    ${PKG_GLIB_LDFLAGS}
  )
endif()

add_executable (aesed aesed.c)
target_link_libraries (aesed PRIVATE
  libbdj4 libbdj4basic libbdj4common
  ${PKG_GCRYPT_LDFLAGS}
  ${PKG_GLIB_LDFLAGS}
)

add_executable (vsed vsed.c)
target_link_libraries (vsed PRIVATE
  libbdj4common
)

add_executable (uitest
  uitest.c
)
target_link_libraries (uitest PRIVATE
  libbdj4uii libbdj4uib ${BDJ4_UI_LIB}
  libbdj4 libbdj4basic libbdj4common
)
addUILibrary (uitest)
addIntlLibrary (uitest)

# want to always build this
if (PKG_LIBVLC_FOUND OR PKG_LIBVLC4_FOUND)
  add_executable (vlcversion
    vlcversion.c
  )
  if (PKG_LIBVLC_FOUND)
    target_include_directories (vlcversion
      PRIVATE "${PKG_LIBVLC_INCLUDE_DIR}"
    )
    target_link_libraries (vlcversion PRIVATE
      ${PKG_LIBVLC_LDFLAGS}
      ${PKG_LIBVLC_LIBRARY}
    )
  endif ()
  if (PKG_LIBVLC4_FOUND)
    target_include_directories (vlcversion
      PRIVATE "${PKG_LIBVLC4_INCLUDE_DIR}"
    )
    target_link_libraries (vlcversion PRIVATE
      ${PKG_LIBVLC4_LDFLAGS}
      ${PKG_LIBVLC4_LIBRARY}
    )
  endif ()
endif()

install (TARGETS
  bdj4info
  bdj4tags
  dmkmfromdb
  tdbcompare
  tdbsetval
  testsuite
  tmusicsetup
  ttagdbchk
  plisinklist
  aesed
  vsed
  uitest
  DESTINATION ${DEST_BIN}
)
if (WIN32)
  install (TARGETS
    bdj4winmksc bdj4winstartup
    DESTINATION ${DEST_BIN}
  )
endif()
if (NOT WIN32 AND NOT APPLE)
  install (TARGETS
    dbustest
    DESTINATION ${DEST_BIN}
  )
endif()
if (PKG_LIBVLC_FOUND OR PKG_LIBVLC4_FOUND)
  install (TARGETS
    vlcversion
    DESTINATION ${DEST_BIN}
  )
endif()

updateRPath (bdj4info)
updateRPath (bdj4tags)
if (WIN32)
  updateRPath (bdj4winmksc)
endif()
updateRPath (dmkmfromdb)
updateRPath (tdbcompare)
updateRPath (tdbsetval)
updateRPath (testsuite)
updateRPath (tmusicsetup)
updateRPath (ttagdbchk)
if (NOT WIN32 AND NOT APPLE)
  updateRPath (dbustest)
endif()
updateRPath (plisinklist)
updateRPath (vsed)
updateRPath (aesed)
updateRPath (uitest)
if (PKG_LIBVLC_FOUND OR PKG_LIBVLC4_FOUND)
  updateRPath (vlcversion)
endif()
