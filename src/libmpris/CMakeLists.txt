#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

if (SYSLINUX)
  # gdbus-codegen claims indicates that --generate-c-code is deprecated,
  # so the command must be run twice for the header and body.
  add_custom_command (
    OUTPUT
      ${PROJECT_SOURCE_DIR}/libmpris/mpris-root.c
      ${PROJECT_SOURCE_DIR}/include/mpris-root.h
    COMMAND
      ${GDBUSCODEGEN}
          --interface-prefix org.mpris.
          --c-namespace mpris
          --header
          --output ${PROJECT_SOURCE_DIR}/include/mpris-root.h
          ${PROJECT_SOURCE_DIR}/libmpris/mpris-root.xml
    COMMAND
      ${GDBUSCODEGEN}
          --interface-prefix org.mpris.
          --c-namespace mpris
          --body
          --output ${PROJECT_SOURCE_DIR}/libmpris/mpris-root.c
          ${PROJECT_SOURCE_DIR}/libmpris/mpris-root.xml
    DEPENDS mpris-root.xml
    DEPENDS ${GDBUSCODEGEN}
  )
  add_custom_command (
    OUTPUT
      ${PROJECT_SOURCE_DIR}/libmpris/mpris-player.c
      ${PROJECT_SOURCE_DIR}/include/mpris-player.h
    COMMAND
      ${GDBUSCODEGEN}
          --interface-prefix org.mpris.
          --c-namespace mpris
          --header
          --output ${PROJECT_SOURCE_DIR}/include/mpris-player.h
          ${PROJECT_SOURCE_DIR}/libmpris/mpris-player.xml
    COMMAND
      ${GDBUSCODEGEN}
          --interface-prefix org.mpris.
          --c-namespace mpris
          --body
          --output ${PROJECT_SOURCE_DIR}/libmpris/mpris-player.c
          ${PROJECT_SOURCE_DIR}/libmpris/mpris-player.xml
    DEPENDS mpris-player.xml
    DEPENDS ${GDBUSCODEGEN}
  )

  add_library (objmprisroot OBJECT mpris-root.c)
  target_include_directories (objmprisroot
    PRIVATE "${PKG_GIO_INCLUDE_DIRS}"
  )
  target_link_libraries (objmprisroot PRIVATE
    ${PKG_GIO_LDFLAGS}
  )

  add_library (objmprisplayer OBJECT mpris-player.c)
  target_include_directories (objmprisplayer
    PRIVATE "${PKG_GIO_INCLUDE_DIRS}"
  )
  target_link_libraries (objmprisplayer PRIVATE
    ${PKG_GIO_LDFLAGS}
  )
endif()
