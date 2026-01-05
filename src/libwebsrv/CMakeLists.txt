#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (objwebsrv OBJECT
  websrv.c ${MONGOOSE_SRC}
)
# mongoose has mixed declarations and statements
target_compile_options (objwebsrv PRIVATE
  "-Wno-declaration-after-statement"
)
target_include_directories (objwebsrv
  PRIVATE "${MONGOOSE_INC_DIR}"
  PRIVATE "${PKG_OPENSSL_INCLUDE_DIRS}"
)
target_link_libraries (objwebsrv PRIVATE
  ${PKG_OPENSSL_LDFLAGS}
)

