#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
  dylib.c
  filemanip.c
  fileshared.c
  log.c
  osdir.c
  oslocale.c
  osrandom.c
  ossignal.c
  pathinfo.c
  queue.c
  roman.c
  sock.c
  sockh.c
  vsencdec.c
)

if (SYSMACOS)
  target_sources (libbdj4common PRIVATE
    osmaclocale.m
  )
  target_link_options (libbdj4common PRIVATE
    -framework Cocoa
  )
endif()
if (SYSWIN)
  target_sources (libbdj4common PRIVATE
    oswinlocale.c
  )
endif()
if (SYSLINUX)
  target_include_directories (libbdj4common
    PRIVATE "${PKG_GIO_INCLUDE_DIRS}"
  )
  target_sources (libbdj4common PRIVATE
    dbusi.c
    oslinuxlocale.c
  )
  target_link_libraries (libbdj4common PRIVATE
    ${PKG_GIO_LDFLAGS}
  )
endif()
target_include_directories (libbdj4common
  PRIVATE "${PKG_GLIB_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4common PRIVATE
  objarg        # bdj4arg.c
  objdirutil    # dirutil.c
  objfileop     # fileop.c
  objlauncher   # pathbld, pathdisp, sysvars,
                # osnetutils, filedata (see ../CMakeLists.txt)
  objosenv      # osenv.c
  objosprocess  # osprocess.c os*process.c
  objosutils    # osutils.c os*utils.c
  objpath       # pathdisp.c, pathutil.c
  objstring     # bdjstring.c stpecpy.c mdebug.c
  objtmutil     # tmutil.c
  ${PKG_GLIB_LDFLAGS}
  ${CMAKE_DL_LIBS}
  m             # needed for Fedora gcc 13
)
addWinSockLibrary (libbdj4common)
# for RtlGetVersion
addWinNtdllLibrary (libbdj4common)
addWinBcryptLibrary (libbdj4common)

install (TARGETS
  libbdj4common
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4common)
