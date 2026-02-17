#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4se bdj4se.c)
target_link_libraries (bdj4se PRIVATE
  objarg
  objdirutil
  objosprocess
  objosenv
  objfileop
  objosutils    # needed for windows
  objstring
  objtmutil
  m
)
if (SYSWIN)
  target_link_options (bdj4se PRIVATE -static-libgcc)
endif()
addIOKitFramework (bdj4se)
addIntlLibrary (bdj4se)
addWinIcon (bdj4se selauncher.rc ${BDJ4INSTICON})
addWinVersionInfo (bdj4se)
addWinManifest (bdj4se)

add_executable (bdj4installer bdj4installer.c locatebdj3.c)
# though a gui program, leave it as console mode
# as that's been tested better, and the command windows
# that pop up are not so random.
target_link_libraries (bdj4installer PRIVATE
  libbdj4uib ${BDJ4_UI_LIB}
  libwebclient libbdj4 libbdj4basic libbdj4common
)
addUILibrary (bdj4installer)
addIntlLibrary (bdj4installer)
addWinVersionInfo (bdj4installer)
addWinManifest (bdj4installer)

add_executable (bdj4cleaninst bdj4cleaninst.c)
target_link_libraries (bdj4cleaninst PRIVATE
  libbdj4uib ${BDJ4_UI_LIB}
  libwebclient libbdj4 libbdj4basic libbdj4common
)
addWinVersionInfo (bdj4cleaninst)
addWinManifest (bdj4cleaninst)

add_executable (bdj4altinst bdj4altinst.c)
target_link_libraries (bdj4altinst PRIVATE
  libbdj4uib ${BDJ4_UI_LIB}
  libwebclient libbdj4 libbdj4basic libbdj4common
)
addUILibrary (bdj4altinst)
addIntlLibrary (bdj4altinst)
addWinVersionInfo (bdj4altinst)
addWinManifest (bdj4altinst)

add_executable (bdj4updater bdj4updater.c)
target_link_libraries (bdj4updater PRIVATE
  libbdj4ati
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
)
addIntlLibrary (bdj4updater)
addWinVersionInfo (bdj4updater)
addWinManifest (bdj4updater)

install (TARGETS
  bdj4se bdj4installer bdj4cleaninst bdj4altinst bdj4updater
  DESTINATION ${DEST_BIN}
)

updateRPath (bdj4installer)
updateRPath (bdj4cleaninst)
updateRPath (bdj4altinst)
updateRPath (bdj4updater)
