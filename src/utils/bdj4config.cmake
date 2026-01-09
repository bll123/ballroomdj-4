#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#

include (CheckCCompilerFlag)
include (CheckLinkerFlag)
include (CheckFunctionExists)
include (CheckLibraryExists)
include (CheckLinkerFlag)
include (CheckSymbolExists)
include (CheckTypeSize)
include (CheckVariableExists)
include (CheckPrototypeDefinition)

find_package (PkgConfig REQUIRED)

find_package (Intl REQUIRED)
find_package (Iconv)

#### BDJ4 UI type

# check for all supported ui interfaces
if (BDJ4_UI STREQUAL "GTK3" OR BDJ4_UI STREQUAL "gtk3" OR
    BDJ4_UI STREQUAL "GTK4" OR BDJ4_UI STREQUAL "gtk4" OR
    BDJ4_UI STREQUAL "NULL" OR BDJ4_UI STREQUAL "null" OR
    BDJ4_UI STREQUAL "macos" OR BDJ4_UI STREQUAL "MacOS" OR
        BDJ4_UI STREQUAL "Macos")
else()
  message (FATAL_ERROR "BDJ4_UI (${BDJ4_UI}) not supported")
endif()

if (BDJ4_UI STREQUAL "GTK3" OR BDJ4_UI STREQUAL "gtk3")
  add_compile_options (-DBDJ4_UI_GTK3=1)
  set (BDJ4_UI_LIB libuigtk3)
endif()

if (BDJ4_UI STREQUAL "GTK4" OR BDJ4_UI STREQUAL "gtk4")
  add_compile_options (-DBDJ4_UI_GTK4=1)
  set (BDJ4_UI_LIB libuigtk4)
endif()

if (BDJ4_UI STREQUAL "NULL" OR BDJ4_UI STREQUAL "null")
  add_compile_options (-DBDJ4_UI_NULL=1)
  set (BDJ4_UI_LIB libuinull)
endif()

if (BDJ4_UI STREQUAL "macos" OR BDJ4_UI STREQUAL "MacOS" OR BDJ4_UI STREQUAL "Macos")
  add_compile_options (-DBDJ4_UI_MACOS=1)
  set (BDJ4_UI_LIB libuimacos)
endif()

#### bits / check supported platforms

if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
  set (BDJ4_BITS 64)
else()
  set (BDJ4_BITS 32)
endif()

if (WIN32 AND BDJ4_BITS EQUAL 32)
  message (FATAL_ERROR "Platform not supported.")
endif()

#### pkg-config / modules

if (BDJ4_MACOS_PKGM STREQUAL "pkgsrc")
  set (ENV{PKG_CONFIG_PATH} "/opt/pkg/lib/${PKGSRC_FFMPEG}/pkgconfig")
endif()

pkg_check_modules (PKG_CHECK check)
pkg_check_modules (PKG_CURL libcurl)
pkg_check_modules (PKG_GCRYPT libgcrypt)
pkg_check_modules (PKG_GIO gio-2.0)
pkg_check_modules (PKG_GLIB glib-2.0)
pkg_check_modules (PKG_JSONC json-c)
pkg_check_modules (PKG_OPENSSL openssl)
pkg_check_modules (PKG_XML2 libxml-2.0)

find_program (GDBUSCODEGEN NAMES gdbus-codegen)

#### UI packages

if (BDJ4_UI STREQUAL "GTK3" OR BDJ4_UI STREQUAL "gtk3")
  pkg_check_modules (PKG_GTK gtk+-3.0)
endif()
if (BDJ4_UI STREQUAL "GTK4" OR BDJ4_UI STREQUAL "gtk4")
  pkg_check_modules (PKG_GTK libgtk-4-1)
endif()

#### volume

# not win and not apple == linux
# if (linux) does not work (2023-4-3)
if (NOT WIN32 AND NOT APPLE)
# not in use
#  pkg_check_modules (PKG_ALSA alsa)
  pkg_check_modules (PKG_PA libpulse)
  pkg_check_modules (PKG_PIPEWIRE libpipewire-0.3)
#  pkg_check_modules (PKG_SPA libspa-0.2)
endif()

#### VLC

# linux
# will need to figure out vlc3/vlc4 in the future
pkg_check_modules (PKG_LIBVLC libvlc)

# on MacOS, the libvlc.dylib is located in different locations
# for vlc-3 and vlc-4.
if (APPLE AND NOT PKG_LIBVLC_FOUND)
  # for development
  if (NOT PKG_LIBVLC_FOUND AND EXISTS "$ENV{HOME}/Applications/VLC3.app/Contents/MacOS/lib/libvlc.dylib")
    message ("-- VLC3: Using $ENV{HOME}/Applications/VLC3.app")
    if (EXISTS "$ENV{HOME}/Applications/VLC3.app/Contents/MacOS/include")
      set (PKG_LIBVLC_INCLUDE_DIR "$ENV{HOME}/Applications/VLC3.app/Contents/MacOS/include")
    else()
      set (PKG_LIBVLC_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-3.0.21")
    endif()
    set (PKG_LIBVLC_LIBRARY "$ENV{HOME}/Applications/VLC3.app/Contents/MacOS/lib/libvlc.dylib")
    set (PKG_LIBVLC_FOUND TRUE)
  endif()
  if (NOT PKG_LIBVLC_FOUND AND EXISTS "/Applications/VLC.app/Contents/MacOS/lib/libvlc.dylib")
    message ("-- VLC3: Using /Applications/VLC.app")
    if (EXISTS "/Applications/VLC3.app/Contents/MacOS/include")
      set (PKG_LIBVLC_INCLUDE_DIR "/Applications/VLC.app/Contents/MacOS/include")
    else()
      set (PKG_LIBVLC_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-3.0.21")
    endif()
    set (PKG_LIBVLC_LIBRARY "/Applications/VLC.app/Contents/MacOS/lib/libvlc.dylib")
    set (PKG_LIBVLC_FOUND TRUE)
  endif()
endif()
if (APPLE AND NOT PKG_LIBVLC4_FOUND)
  # for development
  if (NOT PKG_LIBVLC4_FOUND AND EXISTS "$ENV{HOME}/Applications/VLC4.app/Contents/Frameworks/libvlc.dylib")
    message ("-- VLC4: Using $ENV{HOME}/Applications/VLC4.app")
    set (PKG_LIBVLC4_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-4.0.0")
    set (PKG_LIBVLC4_LIBRARY "$ENV{HOME}/Applications/VLC4.app/Contents/Frameworks/libvlc.dylib")
    set (PKG_LIBVLC4_FOUND TRUE)
  endif()
  if (NOT PKG_LIBVLC4_FOUND AND EXISTS "/Applications/VLC.app/Contents/Frameworks/libvlc.dylib")
    message ("-- VLC4: Using /Applications/VLC.app")
    set (PKG_LIBVLC4_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-4.0.0")
    set (PKG_LIBVLC4_LIBRARY "/Applications/VLC4.app/Contents/Frameworks/libvlc.dylib")
    set (PKG_LIBVLC4_FOUND TRUE)
  endif()
endif()

# The include files are found in the .7z package.
# For the purposes of development,
#   VLC 3 is installed into Program Files/VideoLAN/VLC3
#   VLC 4 is installed into Program Files/VideoLAN/VLC4
# need a way to differentiate a vlc-3 and vlc-4 installation.
if (WIN32 AND NOT PKG_LIBVLC_FOUND)
  if (EXISTS "C:/Program Files/VideoLAN/VLC/libvlc.dll" AND
      NOT EXISTS "C:/Program Files/VideoLAN/VLC3/libvlc.dll")
    message ("-- VLC3: Using C:/Program Files/VideoLAN/VLC")
    set (PKG_LIBVLC_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-3.0.21")
    set (PKG_LIBVLC_LIBRARY "C:/Program Files/VideoLAN/VLC/libvlc.dll")
    set (PKG_LIBVLC_FOUND TRUE)
  endif()
  # for development, override the usual VLC dir, as it is not known
  # what version it is.
  if (NOT PKG_LIBVLC_FOUND AND EXISTS "C:/Program Files/VideoLAN/VLC3/libvlc.dll")
    message ("-- VLC3: Using C:/Program Files/VideoLAN/VLC3")
    set (PKG_LIBVLC_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-3.0.21")
    set (PKG_LIBVLC_LIBRARY "C:/Program Files/VideoLAN/VLC3/libvlc.dll")
    set (PKG_LIBVLC_FOUND TRUE)
  endif()
  # for development
  if (NOT PKG_LIBVLC4_FOUND AND EXISTS "C:/Program Files/VideoLAN/VLC4/libvlc.dll")
    message ("-- VLC4: Using C:/Program Files/VideoLAN/VLC4")
    set (PKG_LIBVLC4_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-4.0.0")
    set (PKG_LIBVLC4_LIBRARY "C:/Program Files/VideoLAN/VLC4/libvlc.dll")
    set (PKG_LIBVLC4_FOUND TRUE)
  endif()
endif()

# gstreamer

pkg_check_modules (PKG_GST gstreamer-1.0)

# VLC is no longer required for a build on all systems
# Windows can use Windows Media Player, and Linux can use GStreamer
if (APPLE AND NOT PKG_LIBVLC_FOUND AND NOT PKG_LIBVLC4_FOUND)
  message (FATAL_ERROR "Unable to locate a VLC library")
endif()
if (NOT APPLE AND NOT WIN32 AND NOT PKG_LIBVLC_FOUND AND NOT PKG_LIBVLC4_FOUND AND NOT PKG_GST_FOUND)
  message (FATAL_ERROR "Unable to locate a VLC/GStreamer library")
endif()

#### tag parsing modules

# ffmpeg : libavformat / libavutil
if (WIN32)
  set (PKG_LIBAVFORMAT_LDFLAGS "${PROJECT_SOURCE_DIR}/../plocal/bin/avformat-61.dll")
#  set (PKG_LIBAVUTIL_LDFLAGS "${PROJECT_SOURCE_DIR}/../plocal/bin/avutil-59.dll")
else()
  pkg_check_modules (PKG_LIBAVFORMAT libavformat)
#  pkg_check_modules (PKG_LIBAVUTIL libavutil)
endif()

# libid3tag
pkg_check_modules (PKG_LIBID3TAG id3tag)

# libvorbisfile, libvorbis, libogg
pkg_check_modules (PKG_LIBVORBISFILE vorbisfile)
pkg_check_modules (PKG_LIBVORBIS vorbis)
pkg_check_modules (PKG_LIBOGG ogg)

# libflac
pkg_check_modules (PKG_LIBFLAC flac)

# libopus
pkg_check_modules (PKG_LIBOPUS opus)
pkg_check_modules (PKG_LIBOPUSFILE opusfile)

# libmp4tag
pkg_check_modules (PKG_LIBMP4TAG libmp4tag)

#### ICU string library

# The ICU library must be pre-compiled and shipped with MacOS and windows
# ICU has incorrect library versioning packaging.
pkg_check_modules (PKG_ICUUC icu-uc)
pkg_check_modules (PKG_ICUI18N icu-i18n)

#### mongoose web server

set (MONGOOSE_INC_DIR ${PROJECT_SOURCE_DIR}/mongoose)
set (MONGOOSE_SRC ${PROJECT_SOURCE_DIR}/mongoose/mongoose.c)

#### generic compile options

macro (checkAddCompileFlag flag)
  # cmake caches every damned variable...
  # need a different variable for every test.
  string (REPLACE "-" "_" tflag ${flag})
  string (REPLACE "=" "_" tflag ${tflag})
  check_c_compiler_flag (${flag} cfchk${tflag})
  if (cfchk${tflag})
    add_compile_options (${flag})
  endif()
endmacro()

macro (checkAddCompileNegFlag flag)
  # cmake caches every damned variable...
  # need a different variable for every test.
  string (REPLACE "-" "_" tflag ${flag})
  string (REPLACE "=" "_" tflag ${tflag})
  string (REPLACE "no-" "" chkflag ${flag})
  check_c_compiler_flag (${chkflag} cfchk${tflag})
  if (cfchk${tflag})
    add_compile_options (${flag})
  endif()
endmacro()

macro (checkAddLinkFlag flag)
  string (REPLACE "-" "_" tflag ${flag})
  string (REPLACE "=" "_" tflag ${tflag})
  check_linker_flag ("C" ${flag} lfchk${tflag})
  if (lfchk${tflag})
    add_link_options (${flag})
  endif()
endmacro()

if (BDJ4_UI STREQUAL "GTK3" OR BDJ4_UI STREQUAL "gtk3" OR
    BDJ4_UI STREQUAL "GTK4" OR BDJ4_UI STREQUAL "gtk4")
  add_compile_options (-DGDK_DISABLE_DEPRECATED)
  add_compile_options (-DGTK_DISABLE_DEPRECATED)
endif()

add_compile_options (-DFF_ENABLE_DEPRECATION_WARNINGS)

checkAddCompileFlag ("-fPIC")
checkAddLinkFlag ("-fPIC")

checkAddCompileFlag ("-Wall")
checkAddCompileFlag ("-Wextra")
# add_compile_options (-Wconversion)
checkAddCompileNegFlag ("-Wno-unused-parameter")
checkAddCompileNegFlag ("-Wno-unknown-pragmas")
checkAddCompileNegFlag ("-Wno-float-equal")
checkAddCompileFlag ("-Wdeclaration-after-statement")
checkAddCompileFlag ("-Wmissing-prototypes")
checkAddCompileFlag ("-Wformat")
# add_compile_options (-Wformat=2)
checkAddCompileFlag ("-Wformat-security")
checkAddCompileFlag ("-Werror=format-security")
checkAddCompileFlag ("-Werror=return-type")
checkAddCompileFlag ("-Wdeprecated-declarations")
checkAddCompileFlag ("-Wunreachable-code")

#### compiler-specific compile options

checkAddCompileFlag ("-Wmaybe-uninitialized")
checkAddCompileNegFlag ("-Wno-unused-but-set-variable")
checkAddCompileNegFlag ("-Wno-stringop-overflow")
checkAddCompileNegFlag ("-Wno-stringop-truncation")
checkAddCompileNegFlag ("-Wno-format-truncation")
checkAddCompileFlag ("-Wno-poison-system-directories")
checkAddCompileNegFlag ("-Wno-shift-sign-overflow")
checkAddCompileNegFlag ("-Wno-pragma-pack")
checkAddCompileNegFlag ("-Wno-ignored-attributes")
checkAddCompileNegFlag ("-Wno-reserved-macro-identifier")
checkAddCompileNegFlag ("-Wno-reserved-id-macro")
checkAddCompileNegFlag ("-Wno-implicit-int-conversion")
checkAddCompileNegFlag ("-Wno-switch-enum")
checkAddCompileNegFlag ("-Wno-gnu-zero-variadic-macro-arguments")
checkAddCompileNegFlag ("-Wno-documentation-deprecated-sync")
checkAddCompileNegFlag ("-Wno-documentation-unknown-command")
checkAddCompileNegFlag ("-Wno-documentation")

#### build compile options

if (BDJ4_BUILD STREQUAL "Release")
  message ("Release Build")
  add_compile_options (-O2)
endif()

if (BDJ4_BUILD STREQUAL "Memdebug")
  message ("Memdebug Build")
  add_compile_options (-DBDJ4_MEM_DEBUG=1)
  add_compile_options (-DBDJ4_LIST_DEBUG=1)
endif()

if (BDJ4_BUILD STREQUAL "Memdebug-Sanitize")
  message ("Memdebug Sanitize Build")
  add_compile_options (-DBDJ4_MEM_DEBUG=1)
  add_compile_options (-DBDJ4_LIST_DEBUG=1)
  add_compile_options (-DBDJ4_USING_SANITIZER=1)
endif()

if (BDJ4_BUILD STREQUAL "SanitizeAddress")
  message ("Sanitize Address Build")
  add_compile_options (-DBDJ4_USING_SANITIZER=1)
endif()

if (BDJ4_BUILD STREQUAL "SanitizeThread")
  message ("Sanitize Thread Build")
endif()

if (BDJ4_BUILD STREQUAL "Debug")
  message ("Debug Build")
  add_compile_options (-Og)
  checkAddCompileFlag ("-ggdb3")
  add_link_options (-g)
endif()

if (BDJ4_BUILD STREQUAL "Profile")
  message ("Profile Build")
  add_compile_options (-O2)
  checkAddCompileFlag ("-pg")
  checkAddLinkFlag ("-pg")
endif()

add_compile_options (-g)
add_link_options (-g)
if (NOT WIN32)
  checkAddLinkFlag ("-rdynamic")
endif()
if (WIN32)
  # msys2 puts the include files for ICU in /usr/include
  add_compile_options (-I/usr/include)
else()
  # suse and fedora put the ffmpeg include files down a level
  add_compile_options (-I/usr/include/ffmpeg)
  add_compile_options (-I${PKG_ICUI18N_INCLUDE_DIRS})
endif()

#### more compile options: fortification/address sanitizer

set (BDJ4_FORTIFY T)

# address sanitizer
if (BDJ4_BUILD STREQUAL "SanitizeAddress" OR
    BDJ4_BUILD STREQUAL "Memdebug-Sanitize" OR
    BDJ4_BUILD STREQUAL "SanitizeThread")
  message ("Sanitize Build")
  set (BDJ4_FORTIFY F)
  add_compile_options (-Og)
#  add_compile_options (-O2)
  checkAddCompileFlag ("-ggdb3")
  add_link_options (-g)
  if (BDJ4_BUILD STREQUAL "SanitizeThread")
    checkAddCompileFlag ("-fsanitize=thread")
    checkAddLinkFlag ("-fsanitize=thread")
  else()
    checkAddCompileFlag ("-fsanitize=address,undefined")
    checkAddLinkFlag ("-fsanitize=address,undefined")
    checkAddCompileFlag ("-fsanitize-address-use-after-scope")
    checkAddLinkFlag ("-fsanitize-address-use-after-scope")
    checkAddCompileFlag ("-fsanitize-recover=address")
    checkAddLinkFlag ("-fsanitize-recover=address")
  endif()
  checkAddCompileFlag ("-fno-omit-frame-pointer")
  checkAddCompileFlag ("-fno-common")
  checkAddCompileFlag ("-fno-inline")
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    checkAddLinkFlag ("-lrt")
  endif()
endif()

if (BDJ4_FORTIFY STREQUAL T)
  # hardening
  checkAddCompileFlag ("-fstack-protector-strong")
  checkAddCompileFlag ("-fstack-protector-all")
  add_compile_options (-D_FORTIFY_SOURCE=2)
else()
  checkAddCompileNegFlag ("-Wno-macro-redefined")
  add_compile_options (-U_FORTIFY_SOURCE)
  add_compile_options (-D_FORTIFY_SOURCE=0)
endif()

#### system specific compile options

if (NOT WIN32)
  if (NOT APPLE)
    SET (CMAKE_INSTALL_RPATH "\${ORIGIN}")
  endif()
  if (APPLE)
    # 10.14 = Mojave, 10.15 = Catalina 11 = Big Sur, 12 = Monterey,
    # 13 = Ventura, 14 = Sonoma, 15 = Sequoia, 26 = Tahoe
    # IMPORTANT: update this in:
    #     pkg/macos/Info.plist
    #     pkg/build/050-id3tag-build.sh
    set (CMAKE_OSX_DEPLOYMENT_TARGET 13)
  endif()

  add_compile_options (-DMG_ARCH=MG_ARCH_UNIX)
endif()
if (WIN32)
  add_compile_options (-DMG_ARCH=MG_ARCH_WIN32)
  add_compile_options (-municode)
endif()

# add_compile_options (-DMG_TLS=MG_TLS_BUILTIN)
add_compile_options (-DMG_TLS=MG_TLS_OPENSSL)

#### checks for include files
# 2025-7-31 replaced with __has_include

#### checks for functions

check_function_exists (__cpuid _lib___cpuid)
set (CMAKE_REQUIRED_LIBRARIES Bcrypt)
check_function_exists (BCryptGenRandom _lib_BCryptGenRandom "-include windows.h -include bcrypt.h")
unset (CMAKE_REQUIRED_LIBRARIES)
check_function_exists (CloseHandle _lib_CloseHandle)
check_function_exists (CompareStringEx _lib_CompareStringEx)
check_function_exists (CopyFileW _lib_CopyFileW)
check_function_exists (CreateFileW _lib_CreateFileW)
check_function_exists (CreateProcessW _lib_CreateProcessW)
check_function_exists (FlushFileBuffers _lib_FlushFileBuffers)
check_function_exists (FindFirstFileW _lib_FindFirstFileW)
check_function_exists (GetCommandLineW _lib_GetCommandLineW)
check_function_exists (GetDateFormatEx _lib_GetDateFormatEx)
check_function_exists (GetFullPathNameW _lib_GetFullPathNameW)
check_function_exists (GetLocaleInfoEx _lib_GetLocaleInfoEx)
check_function_exists (GetNativeSystemInfo _lib_GetNativeSystemInfo)
check_function_exists (GetTimeFormatEx _lib_GetTimeFormatEx)
check_function_exists (GetUserDefaultUILanguage _lib_GetUserDefaultUILanguage)
check_function_exists (LoadLibraryW _lib_LoadLibraryW)
check_function_exists (MultiByteToWideChar _lib_MultiByteToWideChar)
check_function_exists (OpenProcess _lib_OpenProcess)
check_function_exists (ReadFile _lib_ReadFile)
check_function_exists (RemoveDirectoryW _lib_RemoveDirectoryW)
set (CMAKE_REQUIRED_LIBRARIES ntdll)
check_function_exists (RtlGetVersion _lib_RtlGetVersion)
unset (CMAKE_REQUIRED_LIBRARIES)
check_function_exists (SetFilePointer _lib_SetFilePointer)
check_function_exists (Sleep _lib_Sleep)
check_function_exists (TerminateProcess _lib_TerminateProcess)
check_function_exists (WideCharToMultiByte _lib_WideCharToMultiByte)
check_function_exists (WriteFile _lib_WriteFile)
check_function_exists (gmtime_s _lib_gmtime_s)
check_function_exists (localtime_s _lib_localtime_s)
check_function_exists (_sprintf_p _lib__sprintf_p)
check_function_exists (_wchdir _lib__wchdir)
check_function_exists (_wfopen_s _lib__wfopen_s)
check_function_exists (_wgetcwd _lib__wgetcwd)
check_function_exists (_wgetenv_s _lib__wgetenv_s)
check_function_exists (_wputenv _lib__wputenv_s)
check_function_exists (_wrename _lib__wrename)
check_function_exists (_wstat64 _lib__wstat64)
check_function_exists (_wunlink _lib__wunlink)
check_function_exists (_wutime _lib__wutime)

# these do exist
if (WIN32)
  set (_lib_gmtime_s 1)
  set (_lib_localtime_s 1)
  set (_lib__sprintf_p 1)
endif()

set (CMAKE_REQUIRED_LIBRARIES ws2_32)
check_function_exists (ioctlsocket _lib_ioctlsocket)
check_function_exists (select _lib_select)
check_function_exists (socket _lib_socket)
check_function_exists (WSACleanup _lib_WSACleanup)
check_function_exists (WSAGetLastError _lib_WSAGetLastError)
check_function_exists (WSAStartup _lib_WSAStartup)
unset (CMAKE_REQUIRED_LIBRARIES)

set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_DL_LIBS}")
check_symbol_exists (dlopen dlfcn.h _lib_dlopen)
unset (CMAKE_REQUIRED_LIBRARIES)

set (CMAKE_REQUIRED_LIBRARIES pthread)
check_symbol_exists (pthread_create pthread.h _lib_pthread_create)
unset (CMAKE_REQUIRED_LIBRARIES)

if (EXISTS "/opt/local/include" AND
    NOT EXISTS "${PROJECT_SOURCE_DIR}/../devel/macos.pkgsrc" AND
    NOT EXISTS "${PROJECT_SOURCE_DIR}/../devel/macos.homebrew")
  set (CMAKE_REQUIRED_INCLUDES "/opt/local/include")
endif()
if (EXISTS "/opt/pkg/include" AND
    (NOT EXISTS "/opt/local/include" OR
    EXISTS "${PROJECT_SOURCE_DIR}/../devel/macos.pkgsrc"))
  set (CMAKE_REQUIRED_INCLUDES "/opt/pkg/include;/opt/pkg/include/gettext")
endif()
if (EXISTS "/opt/homebrew/include" AND
    (NOT EXISTS "/opt/local/include" OR
    EXISTS "${PROJECT_SOURCE_DIR}/../devel/macos.homebrew"))
  set (CMAKE_REQUIRED_INCLUDES "/opt/homebrew/include")
endif()
if (Intl_LIBRARY AND NOT Intl_LIBRARY STREQUAL "NOTFOUND")
  set (CMAKE_REQUIRED_LIBRARIES "${Intl_LIBRARY}")
endif()
if (Iconv_LIBRARY AND NOT Iconv_LIBRARY STREQUAL "NOTFOUND")
 set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES};${Iconv_LIBRARY}")
endif()
check_symbol_exists (bind_textdomain_codeset libintl.h _lib_bind_textdomain_codeset)
# wbindtextdomain() is needed on windows
# libintl has prefixes, for the check, use the prefixed name
check_symbol_exists (libintl_wbindtextdomain libintl.h _lib_wbindtextdomain)
unset (CMAKE_REQUIRED_LIBRARIES)
unset (CMAKE_REQUIRED_INCLUDES)

check_symbol_exists (backtrace execinfo.h _lib_backtrace)
check_symbol_exists (epoll_create1 sys/epoll.h _lib_epoll_create1)
check_symbol_exists (fcntl fcntl.h _lib_fcntl)
check_symbol_exists (fork unistd.h _lib_fork)
check_symbol_exists (fseeko stdio.h _lib_fseeko)
check_symbol_exists (fsync unistd.h _lib_fsync)
check_symbol_exists (ftello stdio.h _lib_ftello)
check_symbol_exists (getuid unistd.h _lib_getuid)
check_symbol_exists (kill signal.h _lib_kill)
check_symbol_exists (kqueue sys/event.h _lib_kqueue)
check_symbol_exists (epoll_create1 sys/epoll.h _lib_epoll_create1)
check_symbol_exists (localtime_r time.h _lib_localtime_r)
check_symbol_exists (mkdir sys/stat.h _lib_mkdir)
check_symbol_exists (nanosleep time.h _lib_nanosleep)
check_symbol_exists (random stdlib.h _lib_random)
check_symbol_exists (realpath stdlib.h _lib_realpath)
check_symbol_exists (removexattr sys/xattr.h _lib_removexattr)
check_symbol_exists (round math.h _lib_lm)
check_symbol_exists (setenv stdlib.h _lib_setenv)
check_symbol_exists (setrlimit sys/resource.h _lib_setrlimit)
check_symbol_exists (sigaction signal.h _lib_sigaction)
check_symbol_exists (signal signal.h _lib_signal)
check_symbol_exists (srandom stdlib.h _lib_srandom)

# requires _GNU_SOURCE to be declared
set (CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
check_symbol_exists (statx sys/stat.h _lib_statx)
unset (CMAKE_REQUIRED_DEFINITIONS)

check_symbol_exists (stpecpy string.h _lib_stpecpy)

set (CMAKE_REQUIRED_DEFINITIONS "-D_XOPEN_SOURCE")
check_symbol_exists (strptime time.h _lib_strptime)
unset (CMAKE_REQUIRED_DEFINITIONS)

check_symbol_exists (strtok_r string.h _lib_strtok_r)
check_symbol_exists (symlink unistd.h _lib_symlink)
check_symbol_exists (sysconf unistd.h _lib_sysconf)
check_symbol_exists (timegm time.h _lib_timegm)
check_symbol_exists (uname sys/utsname.h _lib_uname)

if (PKG_LIBVLC_FOUND)
  set (CMAKE_REQUIRED_LIBRARIES ${PKG_LIBVLC_LDFLAGS} ${PKG_LIBVLC_LIBRARY})
  check_function_exists (libvlc_new _lib_libvlc3_new)
  unset (CMAKE_REQUIRED_LIBRARIES)
endif()
if (PKG_LIBVLC4_FOUND)
  set (CMAKE_REQUIRED_LIBRARIES ${PKG_LIBVLC4_LDFLAGS} ${PKG_LIBVLC4_LIBRARY})
  check_function_exists (libvlc_new _lib_libvlc4_new)
  unset (CMAKE_REQUIRED_LIBRARIES)
endif()

#### checks for symbols and other stuff

check_prototype_definition (mkdir
    "int mkdir(const char *pathname, mode_t mode)"
    0
    "sys/stat.h;unistd.h"
    temp_mkdir)
if (temp_mkdir)
  set (_args_mkdir 2)
else()
  set (_args_mkdir 1)
endif()

# st_birthtime is a define pointing to a member of the structure
check_symbol_exists (st_birthtime sys/stat.h _mem_struct_stat_st_birthtime)

check_symbol_exists (INVALID_SOCKET "winsock2.h;ws2tcpip.h;windows.h" _define_INVALID_SOCKET)
if (WIN32)
  # another cmake issue
  set (_define_INVALID_SOCKET 1)
endif ()
check_symbol_exists (O_CLOEXEC fcntl.h _define_O_CLOEXEC)
check_symbol_exists (SIGCHLD signal.h _define_SIGCHLD)
check_symbol_exists (SIGHUP signal.h _define_SIGHUP)
check_symbol_exists (S_IRWXU sys/stat.h _define_S_IRWXU)
check_symbol_exists (SO_REUSEPORT sys/socket.h _define_SO_REUSEPORT)
check_symbol_exists (WIFEXITED sys/wait.h _define_WIFEXITED)

check_type_size (suseconds_t _typ_suseconds_t)
set (CMAKE_EXTRA_INCLUDE_FILES winsock2.h ws2tcpip.h windows.h)
check_type_size (HANDLE _typ_HANDLE)
check_type_size (SOCKET _typ_SOCKET)
set (CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
check_type_size (socklen_t _typ_socklen_t)
set (CMAKE_EXTRA_INCLUDE_FILES "")

#### build the config file

configure_file (config.h.in config.h)
if (WIN32)
  configure_file (
      ${PROJECT_SOURCE_DIR}/../pkg/windows/manifest.manifest.in
      manifest.manifest
  )
endif()

cmake_path (SET DEST_BIN NORMALIZE "${PROJECT_SOURCE_DIR}/../bin")
