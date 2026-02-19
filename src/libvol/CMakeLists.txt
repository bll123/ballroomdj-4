#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_library (libbdj4vol SHARED
  volume.c
)
target_link_libraries (libbdj4vol PRIVATE
  libbdj4basic libbdj4common
)

add_library (libvolnull SHARED volnull.c)
target_link_libraries (libvolnull PRIVATE
  objstring
)

if (SYSMACOS)
  add_library (libvolmac SHARED volmac.m)
  target_link_libraries (libvolmac PRIVATE
    "-framework Cocoa" "-framework CoreAudio"
    objstring
  )
endif()

if (SYSLINUX)
  if (PKG_PA_FOUND)
    add_library (libvolpa SHARED volpa.c)
    target_include_directories (libvolpa
      PRIVATE "${PKG_PA_INCLUDE_DIRS}"
    )
    target_link_libraries (libvolpa PRIVATE
      ${PKG_PA_LDFLAGS}
      objstring
    )
  endif()
endif()

if (SYSLINUX)
  if (PKG_PIPEWIRE_FOUND)
    add_library (libvolpipewire SHARED volpipewire.c)
    target_include_directories (libvolpipewire
      PRIVATE "${PKG_PIPEWIRE_INCLUDE_DIRS}"
      PRIVATE "${SPKG_PA_INCLUDE_DIRS}"
    )
    target_link_libraries (libvolpipewire PRIVATE
      ${PKG_PIPEWIRE_LDFLAGS}
      ${SPKG_PA_LDFLAGS}
    )
  endif()
endif()

if (SYSWIN)
  add_library (libvolwin SHARED volwin.cpp)
  target_link_libraries (libvolwin PRIVATE
    objfileop
    objosutils
    objstring
  )
endif()

add_executable (voltest voltest.c)
target_link_libraries (voltest PRIVATE
  libbdj4vol libbdj4basic libbdj4common
)

install (TARGETS
  libbdj4vol libvolnull
  voltest
  DESTINATION ${DEST_BIN}
)
if (SYSLINUX)
  install (TARGETS
    libvolpa
    DESTINATION ${DEST_BIN}
  )
  if (PKG_PIPEWIRE_FOUND)
    install (TARGETS
      libvolpipewire
      DESTINATION ${DEST_BIN}
    )
  endif()
endif()
if (SYSMACOS)
  install (TARGETS
    libvolmac
    DESTINATION ${DEST_BIN}
  )
endif()
if (SYSWIN)
  install (TARGETS
    libvolwin
    DESTINATION ${DEST_BIN}
  )
endif()

updateRPath (libbdj4vol)
updateRPath (voltest)
