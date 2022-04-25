cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4pli SHARED
  pli.c ../dylib.c
)
target_link_libraries (libbdj4pli
  libbdj4common
  m
)

add_library (libplinull SHARED plinull.c)
target_link_libraries (libplinull
  libbdj4common
  m
)

add_library (libplivlc SHARED plivlc.c vlci.c)
target_include_directories (libplivlc
  PUBLIC "${LIBVLC_INCLUDE_DIR}"
)
target_link_libraries (libplivlc
  libbdj4common
  ${LIBVLC_LIBRARY}
  m
)

install (TARGETS
  libbdj4pli libplinull libplivlc
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
