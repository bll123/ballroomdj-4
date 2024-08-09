#
# Copyright 2024 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4cont SHARED
  controller.c
)
target_link_libraries (libbdj4cont PRIVATE
  objdylib
  libbdj4basic libbdj4common
  ${CMAKE_DL_LIBS}
)

if (NOT WIN32 AND NOT APPLE)
  add_custom_command (
    OUTPUT
      ${PROJECT_SOURCE_DIR}/libcont/mpris-root.c
      ${PROJECT_SOURCE_DIR}/libcont/mpris-root.h
    COMMAND
      pwd
    # gdbus-codegen claims indicates that --generate-c-code is deprecated,
    # so the command must be run twice for the header and body.
    COMMAND
      ${GDBUSCODEGEN}
          --interface-prefix org.mpris.
          --c-namespace mpris
          --header
          --output ${PROJECT_SOURCE_DIR}/libcont/mpris-root.h
          ${PROJECT_SOURCE_DIR}/libcont/mpris-root.xml
    COMMAND
      ${GDBUSCODEGEN}
          --interface-prefix org.mpris.
          --c-namespace mpris
          --body
          --output ${PROJECT_SOURCE_DIR}/libcont/mpris-root.c
          ${PROJECT_SOURCE_DIR}/libcont/mpris-root.xml
    DEPENDS mpris-root.xml
  )
  add_custom_command (
    OUTPUT
      ${PROJECT_SOURCE_DIR}/libcont/mpris-player.c
      ${PROJECT_SOURCE_DIR}/libcont/mpris-player.h
    COMMAND
      pwd
    COMMAND
      ${GDBUSCODEGEN}
          --interface-prefix org.mpris.
          --c-namespace mpris
          --header
          --output ${PROJECT_SOURCE_DIR}/libcont/mpris-player.h
          ${PROJECT_SOURCE_DIR}/libcont/mpris-player.xml
    COMMAND
      ${GDBUSCODEGEN}
          --interface-prefix org.mpris.
          --c-namespace mpris
          --body
          --output ${PROJECT_SOURCE_DIR}/libcont/mpris-player.c
          ${PROJECT_SOURCE_DIR}/libcont/mpris-player.xml
    DEPENDS mpris-player.xml
  )

  add_library (libcontmpris SHARED mprisi.c mpris-root.c mpris-player.c)
  target_include_directories (libcontmpris
    PRIVATE "${GLIB_INCLUDE_DIRS}"
  )
  target_link_libraries (libcontmpris PRIVATE
    libbdj4common
    objdbusi
    ${GIO_LDFLAGS}
  )
endif()

install (TARGETS
  libbdj4cont
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (NOT WIN32 AND NOT APPLE)
  install (TARGETS
    libcontmpris
    DESTINATION ${PROJECT_SOURCE_DIR}/../bin
  )
endif()

updateRPath (libbdj4cont)
if (NOT WIN32 AND NOT APPLE)
  updateRPath (libcontmpris)
endif()
