#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

macro (addIntlLibrary name)
  if (Intl_LIBRARY)
    target_link_libraries (${name} PRIVATE
      ${Intl_LIBRARY}
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

macro (updateRPath name)
  if (APPLE)
    set_property (TARGET ${name}
        PROPERTY INSTALL_RPATH
        "@loader_path"
        "@loader_path/../plocal/lib"
    )
    # libicu does not have the proper path when linked
    add_custom_command(TARGET ${name}
        POST_BUILD
        COMMAND
          ${PROJECT_SOURCE_DIR}/utils/macfixrpath.sh
              $<TARGET_FILE:${name}>
        VERBATIM
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
