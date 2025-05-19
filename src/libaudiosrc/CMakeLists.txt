#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

# this object gets duplicated, it's small though.
add_library (objaudiosrcutil OBJECT
  audiosrcutil.c
)

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
  objaudiosrcutil
  libbdj4basic libbdj4common
)

add_library (libasbdj4 SHARED
  audiosrcbdj4.c
)
target_link_libraries (libasbdj4 PRIVATE
  objaudiosrcutil
  libwebclient libbdj4basic libbdj4common
)

add_library (libaspodcast SHARED
  audiosrcpodcast.c rss.c
)
target_link_libraries (libaspodcast PRIVATE
  objaudiosrcutil
  # libbdj4 is needed for tagdef.c
  libwebclient libbdj4 libbdj4basic libbdj4common
)
addIntlLibrary (libaspodcast)

install (TARGETS
  libbdj4audiosrc
  libasbdj4
  libasfile
  libaspodcast
  DESTINATION ${DEST_BIN}
)

updateRPath (libbdj4audiosrc)
updateRPath (libasbdj4)
updateRPath (libasfile)
updateRPath (libaspodcast)
