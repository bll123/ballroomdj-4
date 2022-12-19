#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libwebsrv STATIC
  websrv.c ${MONGOOSE_SRC}
)
target_include_directories (libwebsrv
  PRIVATE "${MONGOOSE_INC_DIR}"
)
