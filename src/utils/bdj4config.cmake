#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#

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
    BDJ4_UI STREQUAL "NULL" OR BDJ4_UI STREQUAL "null")
else()
  message (FATAL_ERROR "BDJ4_UI (${BDJ4_UI}) not supported")
endif()

if (BDJ4_UI STREQUAL "GTK3" OR BDJ4_UI STREQUAL "gtk3")
  add_compile_options (-DBDJ4_USE_GTK3=1)
  set (BDJ4_UI_LIB libuigtk3)
endif()

if (BDJ4_UI STREQUAL "GTK4" OR BDJ4_UI STREQUAL "gtk4")
  add_compile_options (-DBDJ4_USE_GTK4=1)
  set (BDJ4_UI_LIB libuigtk4)
endif()

if (BDJ4_UI STREQUAL "NULL" OR BDJ4_UI STREQUAL "null")
  add_compile_options (-DBDJ4_USE_NULLUI=1)
  set (BDJ4_UI_LIB libuinull)
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

if (BDJ4_UI STREQUAL "GTK3" OR BDJ4_UI STREQUAL "gtk3" OR
    BDJ4_UI STREQUAL "GTK4" OR BDJ4_UI STREQUAL "gtk4")
  add_compile_options (-DGDK_DISABLE_DEPRECATED)
  add_compile_options (-DGTK_DISABLE_DEPRECATED)
endif()

add_compile_options (-DFF_ENABLE_DEPRECATION_WARNINGS)

add_compile_options (-fPIC)

add_compile_options (-Wall)
add_compile_options (-Wextra)
add_compile_options (-Wno-unused-parameter)
add_compile_options (-Wno-unknown-pragmas)
add_compile_options (-Wno-float-equal)
add_compile_options (-Wdeclaration-after-statement)
add_compile_options (-Wmissing-prototypes)
add_compile_options (-Wformat)
add_compile_options (-Wformat-security)
add_compile_options (-Werror=format-security)
add_compile_options (-Werror=return-type)
add_compile_options (-Wdeprecated-declarations)
add_compile_options (-Wunreachable-code)

#### compiler-specific compile options

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_compile_options (-Wmaybe-uninitialized)
  add_compile_options (-Wno-unused-but-set-variable)
  add_compile_options (-Wno-stringop-overflow)
  add_compile_options (-Wno-stringop-truncation)
  add_compile_options (-Wno-format-truncation)
endif()
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  add_compile_options (-Wno-poison-system-directories)
  add_compile_options (-Wno-shift-sign-overflow)
  add_compile_options (-Wno-pragma-pack)
  add_compile_options (-Wno-ignored-attributes)
  if (APPLE)
    add_compile_options (-Wno-reserved-macro-identifier)
  endif()
  add_compile_options (-Wno-reserved-id-macro)
  add_compile_options (-Wno-implicit-int-conversion)
  add_compile_options (-Wno-switch-enum)
  add_compile_options (-Wno-gnu-zero-variadic-macro-arguments)
  add_compile_options (-Wno-documentation-deprecated-sync)
  add_compile_options (-Wno-documentation-unknown-command)
  add_compile_options (-Wno-documentation)
endif()

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
  add_compile_options (-ggdb3)
  add_link_options (-g)
endif()

if (BDJ4_BUILD STREQUAL "Profile")
  message ("Profile Build")
  add_compile_options (-O2)
  add_compile_options (-pg)
  add_link_options (-pg)
endif()

add_compile_options (-g)
add_link_options (-g)
if (NOT WIN32)
  add_link_options (-rdynamic)
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
  add_compile_options (-ggdb3)
  add_link_options (-g)
  add_compile_options (-fsanitize=address)
  add_link_options (-fsanitize=address)
  add_compile_options (-fsanitize-address-use-after-scope)
  add_link_options (-fsanitize-address-use-after-scope)
  add_compile_options (-fsanitize-recover=address)
  add_link_options (-fsanitize-recover=address)
  add_compile_options (-fno-omit-frame-pointer)
  add_compile_options (-fno-common)
  add_compile_options (-fno-inline)
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_link_options (-lrt)
  endif()
endif()
# undef sanitizer
if (BDJ4_BUILD STREQUAL "SanitizeUndef")
  message ("Sanitize Undefined Build")
  set (BDJ4_FORTIFY F)
  add_compile_options (-Og)
  add_compile_options (-ggdb3)
  add_link_options (-g)
  add_compile_options (-fsanitize=undefined)
  add_link_options (-fsanitize=undefined)
  add_compile_options (-fsanitize-undefined-trap-on-error)
  add_compile_options (-fno-omit-frame-pointer)
  add_compile_options (-fno-common)
  add_compile_options (-fno-inline)
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_link_options (-lrt)
  endif()
endif()

if (BDJ4_FORTIFY STREQUAL T)
  # hardening
  add_compile_options (-fstack-protector-strong)
  add_compile_options (-fstack-protector-all)
  add_compile_options (-fstack-protector-strong)
  add_compile_options (-fstack-protector-all)
  add_compile_options (-D_FORTIFY_SOURCE=2)
else()
  if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options (-Wno-macro-redefined)
  endif()
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
    # update this in pkg/MacOS/Info.plist also
    set (CMAKE_OSX_DEPLOYMENT_TARGET 11)
  endif()

  add_compile_options (-DMG_ARCH=MG_ARCH_UNIX)
else()
  add_compile_options (-DMG_ARCH=MG_ARCH_WIN32)
  add_link_options (-static-libgcc)
  add_link_options (-static-libstdc++)
endif()

#### checks for include files

set (CMAKE_REQUIRED_INCLUDES
  /opt/local/include
  ${PROJECT_SOURCE_DIR}/../plocal/include
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
check_include_file (unistd.h _hdr_unistd)
check_include_file (windows.h _hdr_windows)
check_include_file (winsock2.h _hdr_winsock2)
check_include_file (ws2tcpip.h _hdr_ws2tcpip)

set (CMAKE_REQUIRED_INCLUDES ${PIPEWIRE_INCLUDE_DIRS})
check_include_file (pipewire/pipewire.h _hdr_pipewire_pipewire)
# the older version of pipewire does not have spa/utils/json-pod.h
check_include_file (spa/utils/json-pod.h _hdr_spa_utils_jsonpod)
set (CMAKE_REQUIRED_INCLUDES "")

set (CMAKE_REQUIRED_INCLUDES ${GST_INCLUDE_DIRS})
check_include_file (gst/gst.h _hdr_gst_gst)
set (CMAKE_REQUIRED_INCLUDES "")

set (CMAKE_REQUIRED_INCLUDES ${GIO_INCLUDE_DIRS})
check_include_file (gio/gio.h _hdr_gio_gio)
set (CMAKE_REQUIRED_INCLUDES "")

set (CMAKE_REQUIRED_INCLUDES ${LIBVLC_INCLUDE_DIR})
check_include_file (vlc/vlc.h _hdr_vlc_vlc)
set (CMAKE_REQUIRED_INCLUDES "")
set (CMAKE_REQUIRED_INCLUDES ${LIBVLC4_INCLUDE_DIR})
check_include_file (vlc/vlc.h _hdr_vlc_vlc)
set (CMAKE_REQUIRED_INCLUDES "")

set (CMAKE_REQUIRED_INCLUDES ${LIBMPV_INCLUDE_DIRS})
check_include_file (mpv/client.h _hdr_mpv_client)
set (CMAKE_REQUIRED_INCLUDES "")

if (BDJ4_UI STREQUAL "GTK3" OR BDJ4_UI STREQUAL "gtk3" OR
    BDJ4_UI STREQUAL "GTK4" OR BDJ4_UI STREQUAL "gtk4")
  set (CMAKE_REQUIRED_INCLUDES ${GTK_INCLUDE_DIRS})
  check_include_file (gdk/gdkx.h _hdr_gdk_gdkx)
  check_include_file (gtk/gtk.h _hdr_gtk_gtk)
  set (CMAKE_REQUIRED_INCLUDES "")
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

#### checks for functions

set (CMAKE_REQUIRED_INCLUDES windows.h;intrin.h)
check_function_exists (__cpuid _lib___cpuid)
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
check_function_exists (RemoveDirectoryW _lib_RemoveDirectoryW)
# check_function_exists (RtlGetVersion _lib_RtlGetVersion)
check_function_exists (Sleep _lib_Sleep)
check_function_exists (TerminateProcess _lib_TerminateProcess)
check_function_exists (WideCharToMultiByte _lib_WideCharToMultiByte)
check_function_exists (WriteFile _lib_WriteFile)
# check_function_exists (_sprintf_p_lib__sprintf_p)
check_function_exists (_wchdir _lib__wchdir)
check_function_exists (_wfopen _lib__wfopen)
check_function_exists (_wgetcwd _lib__wgetcwd)
check_function_exists (_wgetenv _lib__wgetenv)
check_function_exists (_wputenv _lib__wputenv_s)
check_function_exists (_wrename _lib__wrename)
check_function_exists (_wstat64 _lib__wstat64)
check_function_exists (_wunlink _lib__wunlink)
check_function_exists (_wutime _lib__wutime)
set (CMAKE_REQUIRED_INCLUDES "")

# these do exist
if (WIN32)
  set (_lib_RtlGetVersion 1)
  set (_lib__sprintf_p 1)
endif()

set (CMAKE_REQUIRED_INCLUDES winsock2.h;ws2tcpip.h;windows.h)
set (CMAKE_REQUIRED_LIBRARIES ws2_32)
check_function_exists (ioctlsocket _lib_ioctlsocket)
check_function_exists (select _lib_select)
check_function_exists (socket _lib_socket)
check_function_exists (WSACleanup _lib_WSACleanup)
check_function_exists (WSAGetLastError _lib_WSAGetLastError)
check_function_exists (WSAStartup _lib_WSAStartup)
set (CMAKE_REQUIRED_INCLUDES "")
set (CMAKE_REQUIRED_LIBRARIES "")

set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_DL_LIBS}")
check_function_exists (dlopen _lib_dlopen)
set (CMAKE_REQUIRED_LIBRARIES "")

set (CMAKE_REQUIRED_INCLUDES pthread.h)
set (CMAKE_REQUIRED_LIBRARIES pthread)
check_function_exists (pthread_create _lib_pthread_create)
set (CMAKE_REQUIRED_INCLUDES "")
set (CMAKE_REQUIRED_LIBRARIES "")

set (CMAKE_REQUIRED_INCLUDES libintl.h)
if (Intl_LIBRARY)
   set (CMAKE_REQUIRED_LIBRARIES ${Intl_LIBRARY} ${Iconv_LIBRARY})
endif()
check_function_exists (bind_textdomain_codeset _lib_bind_textdomain_codeset)
# this is needed on windows
# libintl has prefixes, for the check, use the prefixed name
check_function_exists (libintl_wbindtextdomain _lib_wbindtextdomain)
set (CMAKE_REQUIRED_INCLUDES "")
set (CMAKE_REQUIRED_LIBRARIES "")

check_function_exists (backtrace _lib_backtrace)
check_function_exists (drand48 _lib_drand48)
check_function_exists (fcntl _lib_fcntl)
check_function_exists (fork _lib_fork)
check_function_exists (fsync _lib_fsync)
check_function_exists (getuid _lib_getuid)
check_function_exists (ioctlsocket _lib_ioctlsocket)
check_function_exists (kill _lib_kill)
check_function_exists (localtime_r _lib_localtime_r)
check_function_exists (mkdir _lib_mkdir)
check_function_exists (nanosleep _lib_nanosleep)
check_function_exists (rand _lib_rand)
check_function_exists (random _lib_random)
check_function_exists (realpath _lib_realpath)
check_function_exists (removexattr _lib_removexattr)
check_function_exists (round _lib_lm)
check_function_exists (setenv _lib_setenv)
check_function_exists (setrlimit _lib_setrlimit)
check_function_exists (sigaction _lib_sigaction)
check_function_exists (signal _lib_signal)
check_function_exists (srand48 _lib_srand48)
check_function_exists (srand _lib_srand)
check_function_exists (srandom _lib_srandom)
# requires _GNU_SOURCE to be declared, but statx is still located correctly.
check_function_exists (statx _lib_statx)
check_function_exists (strlcat _lib_strlcat)
check_function_exists (strlcpy _lib_strlcpy)
check_function_exists (strptime _lib_strptime)
check_function_exists (strtok_r _lib_strtok_r)
check_function_exists (symlink _lib_symlink)
check_function_exists (sysconf _lib_sysconf)
check_function_exists (timegm _lib_timegm)
check_function_exists (uname _lib_uname)

if (LIBVLC_FOUND)
  set (CMAKE_REQUIRED_LIBRARIES ${LIBVLC_LDFLAGS} ${LIBVLC_LIBRARY})
  check_function_exists (libvlc_new _lib_libvlc3_new)
  set (CMAKE_REQUIRED_LIBRARIES "")
endif()
if (LIBVLC4_FOUND)
  set (CMAKE_REQUIRED_LIBRARIES ${LIBVLC_LDFLAGS} ${LIBVLC4_LIBRARY})
  check_function_exists (libvlc_new _lib_libvlc4_new)
  set (CMAKE_REQUIRED_LIBRARIES "")
endif()

set (CMAKE_REQUIRED_LIBRARIES ${LIBMPV_LDFLAGS})
check_function_exists (mpv_create _lib_mpv_create)
set (CMAKE_REQUIRED_LIBRARIES "")

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

check_symbol_exists (INVALID_SOCKET winsock2.h;ws2tcpip.h;windows.h _define_INVALID_SOCKET)
if (WIN32)
  # another cmake bug
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

