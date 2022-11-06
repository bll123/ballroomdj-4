
macro (addIntlLibrary name)
  if (Intl_LIBRARY)
    target_link_libraries (${name}
      ${Intl_LIBRARY}
      ${Iconv_LIBRARY}
    )
  endif()
endmacro()

macro (addWinSockLibrary name)
  if (WIN32)
    target_link_libraries (${name} ws2_32)
  endif()
endmacro()
# the ntdll library is used for RtlGetVersion()
macro (addWinNtdllLibrary name)
  if (WIN32)
    target_link_libraries (${name} ntdll)
  endif()
endmacro()

macro (macUpdateRPath name)
  add_custom_command(TARGET ${name}
      POST_BUILD
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4.dylib"
            "@executable_path/libbdj4.dylib"
            $<TARGET_FILE:${name}>
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4common.dylib"
            "@executable_path/libbdj4common.dylib"
            $<TARGET_FILE:${name}>
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4basic.dylib"
            "@executable_path/libbdj4basic.dylib"
            $<TARGET_FILE:${name}>
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4pli.dylib"
            "@executable_path/libbdj4pli.dylib"
            $<TARGET_FILE:${name}>
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4ui.dylib"
            "@executable_path/libbdj4ui.dylib"
            $<TARGET_FILE:${name}>
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4vol.dylib"
            "@executable_path/libbdj4vol.dylib"
            $<TARGET_FILE:${name}>
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libuigtk3.dylib"
            "@executable_path/libuigtk3.dylib"
            $<TARGET_FILE:${name}>
      VERBATIM
  )
endmacro()
