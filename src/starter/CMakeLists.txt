#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

# want to keep the launcher at a minimum size and statically linked
# with no shared library dependencies.
add_executable (bdj4 bdj4.c)
addWinIcon (bdj4 launcher.rc ${BDJ4ICON})
target_link_libraries (bdj4 PRIVATE
  objosutils
  objlauncher
  objdirutil
  objosprocess
  objosenv
  objfileop
  objstring
  objtmutil
  m
)
if (WIN32)
  target_link_options (bdj4 PRIVATE -static-libgcc)
endif()
# for gethostname
addIOKitFramework (bdj4)
addIntlLibrary (bdj4)
addWinSockLibrary (bdj4)
addWinNtdllLibrary (bdj4)
addWinVersionInfo (bdj4)
addWinManifest (bdj4)
updateRPath (bdj4)

# this is only used on macos
# the macos application requires an executable linked with the ui framework
if (APPLE)
  add_executable (bdj4g bdj4.c)
  addUIInclude (bdj4g)
  target_compile_options (bdj4g PRIVATE -DBDJ4_GUI_LAUNCHER=1)
  target_link_libraries (bdj4g PRIVATE
    objosutils
    objlauncher
    objdirutil
    objosprocess
    objosenv
    objfileop
    objstring
    objtmutil
    m
  )
  addUILibrary (bdj4g)
  addIOKitFramework (bdj4g)
  addIntlLibrary (bdj4g)
  updateRPath (bdj4g)
endif()

add_executable (bdj4starterui
  bdj4starterui.c
  startutil.c
  support.c
)
if (WIN32)
  target_compile_options (bdj4starterui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4starterui PRIVATE
  libbdj4uib ${BDJ4_UI_LIB}
  libwebclient libbdj4 libbdj4basic libbdj4common
)
addUILibrary (bdj4starterui)
addIntlLibrary (bdj4starterui)
addWinSockLibrary (bdj4starterui)
addWinVersionInfo (bdj4starterui)
addWinManifest (bdj4starterui)
updateRPath (bdj4starterui)

add_executable (bdj4helperui bdj4helperui.c)
if (WIN32)
  target_compile_options (bdj4helperui PRIVATE -mwindows)
endif()
target_link_libraries (bdj4helperui PRIVATE
  libbdj4uib ${BDJ4_UI_LIB}
  libbdj4 libbdj4basic libbdj4common
)
addUILibrary (bdj4helperui)
addIntlLibrary (bdj4helperui)
addWinSockLibrary (bdj4helperui)
addWinVersionInfo (bdj4helperui)
addWinManifest (bdj4helperui)
updateRPath (bdj4helperui)

add_executable (bdj4cleantmp bdj4cleantmp.c)
target_link_libraries (bdj4cleantmp PRIVATE
  libbdj4basic libbdj4common
)
addWinVersionInfo (bdj4cleantmp)
addWinManifest (bdj4cleantmp)
updateRPath (bdj4cleantmp)

install (TARGETS
  bdj4
  bdj4starterui bdj4helperui bdj4cleantmp
  DESTINATION ${DEST_BIN}
)
if (APPLE)
  install (TARGETS
    bdj4g
    DESTINATION ${DEST_BIN}
  )
endif()
