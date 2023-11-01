#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4pli SHARED
  pli.c
)
target_link_libraries (libbdj4pli PUBLIC
  libdylib
  libdyintfc
  libbdj4basic libbdj4common
  m
)

add_library (libplinull SHARED plinull.c)
target_link_libraries (libplinull PUBLIC
  libbdj4common
  m
)

add_library (libplivlc SHARED plivlc.c vlci.c)
target_include_directories (libplivlc
  PUBLIC "${LIBVLC_INCLUDE_DIR}"
)
target_link_libraries (libplivlc PUBLIC
  libbdj4common
  ${LIBVLC_LIBRARY}
  m
)

# add_library (libplimpv SHARED plimpv.c mpvi.c)
# target_include_directories (libplimpv
#   PUBLIC "${LIBMPV_INCLUDE_DIRS}"
# )
# target_link_libraries (libplimpv PUBLIC
#   libbdj4common
#   ${LIBMPV_LDFLAGS}
#   m
# )

install (TARGETS
  libbdj4pli
  libplinull
  libplivlc
#   libplimpv
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (libbdj4pli)
updateRPath (libplinull)
updateRPath (libplivlc)
# updateRPath (libplimpv)
