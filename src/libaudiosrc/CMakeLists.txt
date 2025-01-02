#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4audiosrc SHARED
  audiosrc.c
)
target_link_libraries (libbdj4audiosrc PRIVATE
  objdylib
  libbdj4basic libbdj4common
)

add_library (libasfile SHARED
  audiosrcfile.c
)
target_link_libraries (libasfile PRIVATE
  libbdj4basic libbdj4common
)

install (TARGETS
  libbdj4audiosrc
  libasfile
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4audiosrc)
updateRPath (libasfile)
