#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

include (CheckCCompilerFlag)
include (CheckLinkerFlag)
include (CheckFunctionExists)
include (CheckIncludeFile)
include (CheckIncludeFileCXX)
include (CheckLibraryExists)
include (CheckLinkerFlag)
include (CheckSymbolExists)
include (CheckTypeSize)
include (CheckVariableExists)
include (CheckPrototypeDefinition)

find_package (PkgConfig)

find_package (Intl)
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

# not win and not apple == linux
# if (linux) does not work (2023-4-3)
if (NOT WIN32 AND NOT APPLE)
  pkg_check_modules (ALSA alsa)
endif()
pkg_check_modules (CHECK check)
pkg_check_modules (CURL libcurl)
pkg_check_modules (GCRYPT libgcrypt)
pkg_check_modules (GIO gio-2.0)
pkg_check_modules (GLIB glib-2.0)
pkg_check_modules (JSONC json-c)
pkg_check_modules (LIBSSL libssl)

find_program (GDBUSCODEGEN NAMES gdbus-codegen)

if (BDJ4_UI STREQUAL "GTK3" OR BDJ4_UI STREQUAL "gtk3")
  pkg_check_modules (GTK gtk+-3.0)
endif()
if (BDJ4_UI STREQUAL "GTK4" OR BDJ4_UI STREQUAL "gtk4")
  pkg_check_modules (GTK libgtk-4-1)
endif()
pkg_check_modules (OPENSSL openssl)
if (NOT WIN32 AND NOT APPLE)
  pkg_check_modules (PA libpulse)
  pkg_check_modules (PIPEWIRE libpipewire-0.3)
endif()
pkg_check_modules (XML2 libxml-2.0)

#### VLC

# linux
# will need to figure out vlc3/vlc4 in the future
pkg_check_modules (LIBVLC libvlc)

# on MacOS, the libvlc.dylib is located in a different place
# for vlc-3 and vlc-4.
if (APPLE AND NOT LIBVLC_FOUND)
  if (EXISTS "/Applications/VLC.app/Contents/MacOS/lib/libvlc.dylib")
    message ("-- VLC3: Using /Applications/VLC.app")
    set (LIBVLC_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-3.0.20")
    set (LIBVLC_LIBRARY "/Applications/VLC.app/Contents/MacOS/lib/libvlc.dylib")
    set (LIBVLC_FOUND TRUE)
  endif()
  # for development
  if (NOT LIBVLC_FOUND AND EXISTS "$ENV{HOME}/Applications/VLC3.app/Contents/MacOS/lib/libvlc.dylib")
    message ("-- VLC3: Using $ENV{HOME}/Applications/VLC3.app")
    set (LIBVLC4_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-3.0.20")
    set (LIBVLC4_LIBRARY "/Applications/VLC.app/Contents/MacOS/")
    set (LIBVLC4_FOUND TRUE)
  endif()
endif()
if (APPLE AND NOT LIBVLC4_FOUND)
  if (EXISTS "/Applications/VLC.app/Contents/Frameworks/libvlc.dylib")
    message ("-- VLC4: Using /Applications/VLC.app")
    set (LIBVLC4_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-4.0.0")
    set (LIBVLC4_LIBRARY "/Applications/VLC4.app/Contents/Frameworks/libvlc.dylib")
    set (LIBVLC4_FOUND TRUE)
  endif()
  # for development
  if (NOT LIBVLC4_FOUND AND EXISTS "$ENV{HOME}/Applications/VLC4.app/Contents/Frameworks/libvlc.dylib")
    message ("-- VLC4: Using $ENV{HOME}/Applications/VLC4.app")
    set (LIBVLC4_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-4.0.0")
    set (LIBVLC4_LIBRARY "$ENV{HOME}/Applications/VLC4.app/Contents/Frameworks/libvlc.dylib")
    set (LIBVLC4_FOUND TRUE)
  endif()
endif()

# The include files are found in the .7z package.
# For the purposes of development,
#   VLC 3 is installed into Program Files/VideoLAN/VLC3
#   VLC 4 is installed into Program Files/VideoLAN/VLC4
# need a way to differentiate a vlc-3 and vlc-4 installation.
if (WIN32 AND NOT LIBVLC_FOUND)
  if (EXISTS "C:/Program Files/VideoLAN/VLC/libvlc.dll" AND
      NOT EXISTS "C:/Program Files/VideoLAN/VLC3/libvlc.dll")
    message ("-- VLC3: Using C:/Program Files/VideoLAN/VLC")
    set (LIBVLC_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-3.0.20")
    set (LIBVLC_LIBRARY "C:/Program Files/VideoLAN/VLC/libvlc.dll")
    set (LIBVLC_FOUND TRUE)
  endif()
  # for development, override the usual VLC dir, as it is not known
  # what version it is.
  if (NOT LIBVLC_FOUND AND EXISTS "C:/Program Files/VideoLAN/VLC3/libvlc.dll")
    message ("-- VLC3: Using C:/Program Files/VideoLAN/VLC3")
    set (LIBVLC_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-3.0.20")
    set (LIBVLC_LIBRARY "C:/Program Files/VideoLAN/VLC3/libvlc.dll")
    set (LIBVLC_FOUND TRUE)
  endif()
  # for development
  if (NOT LIBVLC4_FOUND AND EXISTS "C:/Program Files/VideoLAN/VLC4/libvlc.dll")
    message ("-- VLC4: Using C:/Program Files/VideoLAN/VLC4")
    set (LIBVLC4_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/libpli/vlc-4.0.0")
    set (LIBVLC4_LIBRARY "C:/Program Files/VideoLAN/VLC4/libvlc.dll")
    set (LIBVLC4_FOUND TRUE)
  endif()
endif()

# linux can use gstreamer.
if (NOT LIBVLC_FOUND AND NOT LIBVLC4_FOUND)
  if (APPLE OR WIN32)
    message (FATAL_ERROR "Unable to locate a VLC library")
  endif()
endif()

#### MPV
# 2024-2 The MPV interface has issues, and will not be supported at this time.
# pkg_check_modules (LIBMPV mpv)

#### tag parsing modules

# ffmpeg : libavformat / libavutil
if (WIN32)
  set (LIBAVFORMAT_LDFLAGS "${PROJECT_SOURCE_DIR}/../plocal/bin/avformat-61.dll")
  set (LIBAVUTIL_LDFLAGS "${PROJECT_SOURCE_DIR}/../plocal/bin/avutil-59.dll")
else()
  pkg_check_modules (LIBAVFORMAT libavformat)
  pkg_check_modules (LIBAVUTIL libavutil)
endif()

# gstreamer

pkg_check_modules (GST gstreamer-1.0)

# libid3tag
pkg_check_modules (LIBID3TAG id3tag)

# libvorbisfile, libvorbis, libogg
pkg_check_modules (LIBVORBISFILE vorbisfile)
pkg_check_modules (LIBVORBIS vorbis)
pkg_check_modules (LIBOGG ogg)

# libflac
pkg_check_modules (LIBFLAC flac)

# libopus
pkg_check_modules (LIBOPUS opus)
pkg_check_modules (LIBOPUSFILE opusfile)

# libmp4tag
pkg_check_modules (LIBMP4TAG libmp4tag)

#### ICU string library

# The ICU library must be pre-compiled and shipped with Linux and MacOS.
# ICU has incorrect library versioning packaging.
pkg_check_modules (ICUI18N icu-i18n)

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
checkAddCompileFlag ("-Wno-unused-parameter")
checkAddCompileFlag ("-Wno-unknown-pragmas")
checkAddCompileFlag ("-Wno-float-equal")
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
checkAddCompileFlag ("-Wno-unused-but-set-variable")
checkAddCompileFlag ("-Wno-stringop-overflow")
checkAddCompileFlag ("-Wno-stringop-truncation")
checkAddCompileFlag ("-Wno-format-truncation")
checkAddCompileFlag ("-Wno-poison-system-directories")
checkAddCompileFlag ("-Wno-shift-sign-overflow")
checkAddCompileFlag ("-Wno-pragma-pack")
checkAddCompileFlag ("-Wno-ignored-attributes")
checkAddCompileFlag ("-Wno-reserved-macro-identifier")
checkAddCompileFlag ("-Wno-reserved-id-macro")
checkAddCompileFlag ("-Wno-implicit-int-conversion")
checkAddCompileFlag ("-Wno-switch-enum")
checkAddCompileFlag ("-Wno-gnu-zero-variadic-macro-arguments")
checkAddCompileFlag ("-Wno-documentation-deprecated-sync")
checkAddCompileFlag ("-Wno-documentation-unknown-command")
checkAddCompileFlag ("-Wno-documentation")

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

if (BDJ4_BUILD STREQUAL "Debug")
  message ("Debug Build")
  add_compile_options (-Og)
  add_compile_options (-O0)
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
  add_compile_options (-I${ICUI18N_INCLUDE_DIRS})
endif()

#### more compile options: fortification/address sanitizer

set (BDJ4_FORTIFY T)

# address sanitizer
if (BDJ4_BUILD STREQUAL "SanitizeAddress" OR BDJ4_BUILD STREQUAL "Memdebug-Sanitize")
  message ("Sanitize Address Build")
  set (BDJ4_FORTIFY F)
  add_compile_options (-Og)
  checkAddCompileFlag ("-ggdb3")
  add_link_options (-g)
  checkAddCompileFlag ("-fsanitize=address")
  checkAddLinkFlag ("-fsanitize=address")
  checkAddCompileFlag ("-fsanitize-address-use-after-scope")
  checkAddLinkFlag ("-fsanitize-address-use-after-scope")
  checkAddCompileFlag ("-fsanitize-recover=address")
  checkAddLinkFlag ("-fsanitize-recover=address")
  checkAddCompileFlag ("-fno-omit-frame-pointer")
  checkAddCompileFlag ("-fno-common")
  checkAddCompileFlag ("-fno-inline")
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    checkAddLinkFlag ("-lrt")
  endif()
endif()
# undef sanitizer
if (BDJ4_BUILD STREQUAL "SanitizeUndef")
  message ("Sanitize Undefined Build")
  set (BDJ4_FORTIFY F)
  add_compile_options (-Og)
  checkAddCompileFlag ("-ggdb3")
  add_link_options (-g)
  checkAddCompileFlag ("-fsanitize=undefined")
  checkAddLinkFlag ("-fsanitize=undefined")
  checkAddCompileFlag ("-fsanitize-undefined-trap-on-error")
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
  checkAddCompileFlag ("-Wno-macro-redefined")
  add_compile_options (-U_FORTIFY_SOURCE)
  add_compile_options (-D_FORTIFY_SOURCE=0)
endif()

#### system specific compile options

if (NOT WIN32)
  if (NOT APPLE)
    SET (CMAKE_INSTALL_RPATH "\${ORIGIN}")
  endif()
  if (APPLE)
    # 10.14 = Mojave, 10.15 = Catalina
    # 11 = Big Sur, 12 = Monterey, 13 = Ventura, 14 = Sonoma
    # 15 = Sequoia
    # IMPORTANT: update this in pkg/macos/Info.plist also
    set (CMAKE_OSX_DEPLOYMENT_TARGET 11)
  endif()

  add_compile_options (-DMG_ARCH=MG_ARCH_UNIX)
endif()
if (WIN32)
  add_compile_options (-DMG_ARCH=MG_ARCH_WIN32)
  checkAddLinkFlag ("-static-libgcc")
endif()

# add_compile_options (-DMG_TLS=MG_TLS_BUILTIN)
add_compile_options (-DMG_TLS=MG_TLS_OPENSSL)

#### checks for include files

set (CMAKE_REQUIRED_INCLUDES
  ${PROJECT_SOURCE_DIR}/../plocal/include
  /opt/local/include
)

check_include_file (alsa/asoundlib.h _hdr_alsa_asoundlib)
check_include_file (arpa/inet.h _hdr_arpa_inet)
check_include_file (dlfcn.h _hdr_dlfcn)
check_include_file (endpointvolume.h _hdr_endpointvolume)
check_include_file (execinfo.h _hdr_execinfo)
check_include_file (fcntl.h _hdr_fcntl)
check_include_file (intrin.h _hdr_intrin)
check_include_file (io.h _hdr_io)
check_include_file (libintl.h _hdr_libintl)
check_include_file (MacTypes.h _hdr_MacTypes)
check_include_file (math.h _hdr_math)
check_include_file (netdb.h _hdr_netdb)
check_include_file (netinet/in.h _hdr_netinet_in)
check_include_file (poll.h _hdr_poll)
check_include_file (pthread.h _hdr_pthread)
check_include_file (pulse/pulseaudio.h _hdr_pulse_pulseaudio)
check_include_file (signal.h _hdr_signal)
check_include_file (stdatomic.h _hdr_stdatomic)
check_include_file (stdint.h _hdr_stdint)
check_include_file (string.h _hdr_string)
check_include_file (tchar.h _hdr_tchar)
check_include_file (unistd.h _hdr_unistd)
check_include_file (windows.h _hdr_windows)
check_include_file (winsock2.h _hdr_winsock2)
check_include_file (ws2tcpip.h _hdr_ws2tcpip)

# bcrypt requires windows.h
check_include_file (bcrypt.h _hdr_bcrypt "-include windows.h")

set (CMAKE_REQUIRED_INCLUDES ${PIPEWIRE_INCLUDE_DIRS})
check_include_file (pipewire/pipewire.h _hdr_pipewire_pipewire)
# the older version of pipewire does not have spa/utils/json-pod.h
check_include_file (spa/utils/json-pod.h _hdr_spa_utils_jsonpod)
unset (CMAKE_REQUIRED_INCLUDES)

set (CMAKE_REQUIRED_INCLUDES ${GST_INCLUDE_DIRS})
check_include_file (gst/gst.h _hdr_gst_gst)
unset (CMAKE_REQUIRED_INCLUDES)

set (CMAKE_REQUIRED_INCLUDES ${GIO_INCLUDE_DIRS})
check_include_file (gio/gio.h _hdr_gio_gio)
unset (CMAKE_REQUIRED_INCLUDES)

set (CMAKE_REQUIRED_INCLUDES ${LIBVLC_INCLUDE_DIR})
check_include_file (vlc/vlc.h _hdr_vlc_vlc)
unset (CMAKE_REQUIRED_INCLUDES)
set (CMAKE_REQUIRED_INCLUDES ${LIBVLC4_INCLUDE_DIR})
check_include_file (vlc/vlc.h _hdr_vlc_vlc)
unset (CMAKE_REQUIRED_INCLUDES)

set (CMAKE_REQUIRED_INCLUDES ${LIBMPV_INCLUDE_DIRS})
check_include_file (mpv/client.h _hdr_mpv_client)
unset (CMAKE_REQUIRED_INCLUDES)

if (BDJ4_UI STREQUAL "GTK3" OR BDJ4_UI STREQUAL "gtk3" OR
    BDJ4_UI STREQUAL "GTK4" OR BDJ4_UI STREQUAL "gtk4")
  set (CMAKE_REQUIRED_INCLUDES ${GTK_INCLUDE_DIRS})
  check_include_file (gdk/gdkx.h _hdr_gdk_gdkx)
  check_include_file (gtk/gtk.h _hdr_gtk_gtk)
  unset (CMAKE_REQUIRED_INCLUDES)
endif()

check_include_file (sys/resource.h _sys_resource)
check_include_file (sys/select.h _sys_select)
check_include_file (sys/signal.h _sys_signal)
check_include_file (sys/socket.h _sys_socket)
check_include_file (sys/stat.h _sys_stat)
check_include_file (sys/time.h _sys_time)
check_include_file (sys/utsname.h _sys_utsname)
check_include_file (sys/wait.h _sys_wait)
check_include_file (sys/xattr.h _sys_xattr)

unset (CMAKE_REQUIRED_INCLUDES)

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

set (CMAKE_REQUIRED_INCLUDES "/opt/local/include")
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
check_symbol_exists (fcntl fcntl.h _lib_fcntl)
check_symbol_exists (fork unistd.h _lib_fork)
check_symbol_exists (fsync unistd.h _lib_fsync)
check_symbol_exists (getuid unistd.h _lib_getuid)
check_symbol_exists (kill signal.h _lib_kill)
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

if (LIBVLC_FOUND)
  set (CMAKE_REQUIRED_LIBRARIES ${LIBVLC_LDFLAGS} ${LIBVLC_LIBRARY})
  check_function_exists (libvlc_new _lib_libvlc3_new)
  unset (CMAKE_REQUIRED_LIBRARIES)
endif()
if (LIBVLC4_FOUND)
  set (CMAKE_REQUIRED_LIBRARIES ${LIBVLC_LDFLAGS} ${LIBVLC4_LIBRARY})
  check_function_exists (libvlc_new _lib_libvlc4_new)
  unset (CMAKE_REQUIRED_LIBRARIES)
endif()

set (CMAKE_REQUIRED_LIBRARIES ${LIBMPV_LDFLAGS})
check_function_exists (mpv_create _lib_mpv_create)
unset (CMAKE_REQUIRED_LIBRARIES)

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
check_symbol_exists (SOCK_CLOEXEC sys/socket.h _define_SOCK_CLOEXEC)
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
