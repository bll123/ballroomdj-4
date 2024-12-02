#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4common SHARED
  bdjmsg.c
  bdjregex.c
  bdjvars.c
  callback.c
  colorutils.c
  conn.c
  dirop.c
  filemanip.c
  fileshared.c
  log.c
  osdir.c
  oslinuxlocale.c
  oslocale.c
  osmaclocale.m
  oswinlocale.c
  osrandom.c
  ossignal.c
  pathinfo.c
  queue.c
  roman.c
  sock.c
  sockh.c
  vsencdec.c
)
target_include_directories (libbdj4common
  PRIVATE "${GLIB_INCLUDE_DIRS}"
)
if (APPLE)
  target_link_options (libbdj4common PRIVATE
    -framework Cocoa
  )
endif()
target_link_libraries (libbdj4common PRIVATE
  objosutils    # osutils.c os*utils.c
  objlauncher   # bdj4arg, pathbld, pathdisp, sysvars, pathutil,
                # osnetutils, filedata (see ../CMakeLists.txt)
  objosenv      # osenv.c
  objdirutil    # dirutil.c
  objfileop     # fileop.c
  objosprocess  # osprocess.c os*process.c
  objstring     # bdjstring.c stpecpy.c strlcat.c strlcpy.c mdebug.c
  objtmutil     # tmutil.c
  ${GLIB_LDFLAGS}
  m             # needed for Fedora gcc 13
)
addIOKitFramework (libbdj4common)
addIntlLibrary (libbdj4common)
addWinSockLibrary (libbdj4common)
# for RtlGetVersion
addWinNtdllLibrary (libbdj4common)
addWinBcryptLibrary (libbdj4common)

install (TARGETS
  libbdj4common
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4common)
