#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (objwebsrv OBJECT
  websrv.c ${MONGOOSE_SRC}
)
target_include_directories (objwebsrv
  PRIVATE "${MONGOOSE_INC_DIR}"
  PRIVATE "${LIBSSL_INCLUDE_DIRS}"
)
target_link_libraries (objwebsrv PRIVATE
  ${LIBSSL_LDFLAGS}
)

