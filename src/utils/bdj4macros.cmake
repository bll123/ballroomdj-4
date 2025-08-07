#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

macro (addIntlLibrary name)
  if (Intl_LIBRARY AND NOT Intl_LIBRARY STREQUAL "NOTFOUND")
    target_link_libraries (${name} PRIVATE
      ${Intl_LIBRARY}
    )
  endif()
  if (Iconv_LIBRARY AND NOT Iconv_LIBRARY STREQUAL "NOTFOUND")
    target_link_libraries (${name} PRIVATE
      ${Iconv_LIBRARY}
    )
  endif()
endmacro()

macro (addIOKitFramework name)
  if (APPLE)
    target_link_libraries (${name} PRIVATE
      "-framework IOKit" "-framework CoreFoundation"
    )
  endif()
endmacro()

macro (addWinSockLibrary name)
  if (WIN32)
    target_link_libraries (${name} PRIVATE ws2_32)
  endif()
endmacro()

# the ntdll library is used for RtlGetVersion()
macro (addWinNtdllLibrary name)
  if (WIN32)
    target_link_libraries (${name} PRIVATE ntdll)
  endif()
endmacro()

macro (addWinBcryptLibrary name)
  if (WIN32)
    target_link_libraries (${name} PRIVATE Bcrypt)
  endif()
endmacro()

macro (addWinIcon name rcnm iconnm)
  if (WIN32)
    add_custom_command (
      OUTPUT ${rcnm}
      COMMAND cp -f ${PROJECT_SOURCE_DIR}/../img/${iconnm} .
      COMMAND echo "id ICON ${iconnm}" > ${rcnm}
      MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/../img/${iconnm}
      BYPRODUCTS ${iconnm}
      VERBATIM
    )
    target_sources (${name} PRIVATE ${rcnm})
  endif()
endmacro()

macro (addWinVersionInfo name)
  if (WIN32)
    set (tversfn ${name})
    # this will fail for x.x.x.x forms.
    string (REPLACE "." "," tbdj4vers "${BDJ4_VERSION}")
    string (REGEX REPLACE "[^,]" "" tempvi ${tbdj4vers})
    string (LENGTH ${tempvi} tempvilen)
    if (${tempvilen} EQUAL 2)
      string (APPEND tbdj4vers ",0")
    endif()
    configure_file (
      ${PROJECT_SOURCE_DIR}/../pkg/windows/version.rc.in
      ${name}-version.rc
    )
    target_sources (${name} PRIVATE ${name}-version.rc)
  endif()
endmacro()

macro (addWinManifest name)
  if (WIN32)
    set (t${name}man "${name}.manifest")
    add_custom_command (
      OUTPUT ${name}.rc
      COMMAND cp -f ${PROJECT_BINARY_DIR}/manifest.manifest ${t${name}man}
      COMMAND echo "1 RT_MANIFEST \"${t${name}man}\"" > ${name}.rc
      MAIN_DEPENDENCY ${PROJECT_BINARY_DIR}/manifest.manifest
      BYPRODUCTS ${t${name}man}
      VERBATIM
    )
    target_sources (${name} PRIVATE ${name}.rc)
  endif()
endmacro()

macro (updateRPath name)
  if (APPLE)
    set_property (TARGET ${name}
        PROPERTY INSTALL_RPATH
        "@loader_path"
        "@loader_path/../plocal/lib"
    )
  endif()
  if (NOT APPLE AND NOT WIN32)
    set_property (TARGET ${name}
        PROPERTY INSTALL_RPATH
        "\${ORIGIN}"
        "\${ORIGIN}/../plocal/lib"
        "\${ORIGIN}/../plocal/lib64"
    )
  endif()
endmacro()
