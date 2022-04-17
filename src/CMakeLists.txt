cmake_minimum_required (VERSION 3.18)

# avoid msys2/windows issue
set (CMAKE_C_COMPILER_WORKS 1)
set (CMAKE_CXX_COMPILER_WORKS 1)

set (BDJ4_VERSION 4.0.0)
project (BDJ4 VERSION ${BDJ4_VERSION})
set (BDJ4ICON bdj4_icon.ico)
set (BDJ4INSTICON bdj4_icon_inst.ico)

set (default_build_type "Debug")

set (CMAKE_MODULE_PATH utils)
set (CMAKE_SHARED_LIBRARY_PREFIX "")
set (CMAKE_STATIC_LIBRARY_PREFIX "")
set (CMAKE_INSTALL_PREFIX "")
set (CMAKE_PREFIX_PATH /opt/local;${PROJECT_SOURCE_DIR}/../plocal)

# this doesn't seem to help with locating header files...
include_directories (
  "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
  ${PROJECT_SOURCE_DIR}/../plocal/include
)

include_directories (
  SYSTEM /opt/local/include /usr/local/include
)

# This seems to work on linux.
# cmake uses '@rpath' on macos -- does not.
SET (CMAKE_SKIP_BUILD_RPATH FALSE)
SET (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set (SHLIB_EXT ${CMAKE_SHARED_LIBRARY_SUFFIX})
set (MONGOOSE_INC_DIR ../packages/mongoose)
set (MONGOOSE_SRC ../packages/mongoose/mongoose.c)

include (CheckFunctionExists)
include (CheckIncludeFile)
include (CheckIncludeFileCXX)
# include (CheckIncludeFiles)
include (CheckLibraryExists)
include (CheckLinkerFlag)
include (CheckSymbolExists)
include (CheckTypeSize)
include (CheckVariableExists)
include (CheckPrototypeDefinition)

find_package (PkgConfig)

find_package (Intl)
find_package (Iconv)

# does not work on windows. find_library() fails.
find_package (LIBVLC)

# so hard-code the paths.
# the library path will always be valid.
if (NOT LIBVLC_LIBRARY)
  set (LIBVLC_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/vlc-3.0.16")
  set (LIBVLC_LIBRARY "C:/Program Files/VideoLAN/VLC/libvlc.dll")
  set (LIBVLC_FOUND TRUE)
endif()

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

pkg_check_modules (OPENSSL openssl)
pkg_check_modules (CURL libcurl)
pkg_check_modules (ALSA alsa)
pkg_check_modules (GTK gtk+-3.0)
pkg_check_modules (GLIB glib-2.0)
pkg_check_modules (CHECK check)
pkg_check_modules (PA libpulse)

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

add_compile_options (-DGDK_DISABLE_DEPRECATED)
add_compile_options (-DGTK_DISABLE_DEPRECATED)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
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

if (BDJ4_BUILD STREQUAL "Release")
  message ("Debug Build")
endif()

if (BDJ4_BUILD STREQUAL "Debug")
  message ("Debug Build")
  add_compile_options (-O0)
  add_compile_options (-g)
  add_link_options (-g)
endif()

set (BDJ4_NO_FORTIFY F)

# address sanitizer
if (BDJ4_BUILD STREQUAL "SanitizeAddress")
  message ("Sanitize Address Build")
  set (BDJ4_NO_FORTIFY T)
  add_compile_options (-O0)
  add_compile_options (-g)
  add_link_options (-g)
  add_compile_options (-fsanitize=address)
  add_compile_options (-fsanitize-address-use-after-scope)
  add_link_options (-fsanitize=address)
  add_compile_options (-ggdb)
  add_compile_options (-fno-omit-frame-pointer)
  add_compile_options (-fno-common)
  add_compile_options (-fno-inline)
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_link_options (-lrt)
  endif()
endif()
# leak sanitizer
if (BDJ4_BUILD STREQUAL "SanitizeLeak")
  message ("Sanitize Leak Build")
  set (BDJ4_NO_FORTIFY T)
  add_compile_options (-O0)
  add_compile_options (-g)
  add_link_options (-g)
  add_compile_options (-fsanitize=leak)
  add_link_options (-fsanitize=leak)
  add_compile_options (-ggdb)
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
  set (BDJ4_NO_FORTIFY T)
  add_compile_options (-O0)
  add_compile_options (-g)
  add_link_options (-g)
  add_compile_options (-fsanitize=undefined)
  add_link_options (-fsanitize=undefined)
  add_compile_options (-fsanitize-undefined-trap-on-error)
  add_compile_options (-ggdb)
  add_compile_options (-fno-omit-frame-pointer)
  add_compile_options (-fno-common)
  add_compile_options (-fno-inline)
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_link_options (-lrt)
  endif()
endif()

if (NOT WIN32)
  SET (CMAKE_INSTALL_RPATH "\${ORIGIN}")
  if (APPLE)
    add_compile_options (-mmacosx-version-min=10.13)
    # does not work.
    # SET (CMAKE_INSTALL_RPATH "@executable_path")
  endif()

  add_compile_options (-DMG_ARCH=MG_ARCH_UNIX)

  if (BDJ4_NO_FORTIFY STREQUAL T)
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
else()
  add_compile_options (-DMG_ARCH=MG_ARCH_WIN32)
  add_compile_options (-D_FORTIFY_SOURCE=0)
  add_link_options (-static-libgcc)
endif()

set (CMAKE_REQUIRED_INCLUDES
  /opt/local/include
  ${PROJECT_SOURCE_DIR}/../plocal/include
)

check_include_file (alsa/asoundlib.h _hdr_alsa_asoundlib)
check_include_file (arpa/inet.h _hdr_arpa_inet)
check_include_file (dlfcn.h _hdr_dlfcn)
check_include_file (endpointvolume.h _hdr_endpointvolume)
check_include_file (fcntl.h _hdr_fcntl)
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
check_include_file (stdint.h _hdr_stdint)
check_include_file (string.h _hdr_string)
check_include_file (unistd.h _hdr_unistd)
check_include_file (winsock2.h _hdr_winsock2)
check_include_file (ws2tcpip.h _hdr_ws2tcpip)
check_include_file (windows.h _hdr_windows)

set (CMAKE_REQUIRED_INCLUDES ${LIBVLC_INCLUDE_DIR})
check_include_file (vlc/vlc.h _hdr_vlc_vlc)
set (CMAKE_REQUIRED_INCLUDES "")

set (CMAKE_REQUIRED_INCLUDES ${GTK_INCLUDE_DIRS})
check_include_file (gtk/gtk.h _hdr_gtk_gtk)
set (CMAKE_REQUIRED_INCLUDES "")

check_include_file (sys/resource.h _sys_resource)
check_include_file (sys/select.h _sys_select)
check_include_file (sys/signal.h _sys_signal)
check_include_file (sys/socket.h _sys_socket)
check_include_file (sys/stat.h _sys_stat)
check_include_file (sys/time.h _sys_time)
check_include_file (sys/utsname.h _sys_utsname)
check_include_file (sys/wait.h _sys_wait)

set (CMAKE_REQUIRED_INCLUDES windows.h)
check_function_exists (CloseHandle _lib_CloseHandle)
check_function_exists (CopyFileW _lib_CopyFileW)
check_function_exists (CreateFile _lib_CreateFile)
check_function_exists (CreateProcess _lib_CreateProcess)
check_function_exists (FindFirstFileW _lib_FindFirstFileW)
check_function_exists (GetFullPathName _lib_GetFullPathName)
check_function_exists (RtlGetVersion _lib_RtlGetVersion)
check_function_exists (LoadLibrary _lib_LoadLibrary)
check_function_exists (MultiByteToWideChar _lib_MultiByteToWideChar)
check_function_exists (OpenProcess _lib_OpenProcess)
check_function_exists (TerminateProcess _lib_TerminateProcess)
check_function_exists (WideCharToMultiByte _lib_WideCharToMultiByte)
check_function_exists (WriteFile _lib_WriteFile)
check_function_exists (_wfopen _lib__wfopen)
check_function_exists (_wstat _lib__wstat)
check_function_exists (_wunlink _lib__wunlink)
set (CMAKE_REQUIRED_INCLUDES "")

# cmake weirdness
# these do exist
if (WIN32)
  set (_lib_CreateFile 1)
  set (_lib_CreateProcess 1)
  set (_lib_GetFullPathName 1)
  set (_lib_RtlGetVersion 1)
  set (_lib_LoadLibrary 1)
endif()

set (CMAKE_REQUIRED_INCLUDES winsock2.h;windows.h)
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
set (CMAKE_REQUIRED_INCLUDES "")
set (CMAKE_REQUIRED_LIBRARIES "")

check_function_exists (ioctlsocket _lib_ioctlsocket)
check_function_exists (drand48 _lib_drand48)
check_function_exists (fcntl _lib_fcntl)
check_function_exists (fork _lib_fork)
check_function_exists (kill _lib_kill)
check_function_exists (localtime_r _lib_localtime_r)
check_function_exists (mkdir _lib_mkdir)
check_function_exists (rand _lib_rand)
check_function_exists (random _lib_random)
check_function_exists (realpath _lib_realpath)
check_function_exists (round _lib_lm)
check_function_exists (setenv _lib_setenv)
check_function_exists (setrlimit _lib_setrlimit)
check_function_exists (sigaction _lib_sigaction)
check_function_exists (signal _lib_signal)
check_function_exists (srand48 _lib_srand48)
check_function_exists (srandom _lib_srandom)
check_function_exists (srand _lib_srand)
check_function_exists (strlcpy _lib_strlcpy)
check_function_exists (strlcat _lib_strlcat)
check_function_exists (strtok_r _lib_strtok_r)
check_function_exists (symlink _lib_symlink)
check_function_exists (uname _lib_uname)

set (CMAKE_REQUIRED_LIBRARIES ${LIBVLC_LIBRARY})
check_function_exists (libvlc_new _lib_libvlc_new)
check_function_exists (libvlc_audio_output_device_enum _lib_libvlc_audio_output_device_enum)
set (CMAKE_REQUIRED_LIBRARIES "")

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

check_symbol_exists (INVALID_SOCKET winsock2.h;windows.h _define_INVALID_SOCKET)
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

check_type_size (suseconds_t _typ_suseconds_t)
set (CMAKE_EXTRA_INCLUDE_FILES winsock2.h windows.h)
check_type_size (HANDLE _typ_HANDLE)
check_type_size (SOCKET _typ_SOCKET)
set (CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
check_type_size (socklen_t _typ_socklen_t)
set (CMAKE_EXTRA_INCLUDE_FILES "")

configure_file (config.h.in config.h)

add_library (libbdj4common SHARED
  bdjmsg.c
  bdjopt.c
  bdjstring.c
  bdjvars.c
  conn.c
  datafile.c
  filedata.c
  filemanip.c
  fileop.c
  fileutil.c
  ilist.c
  list.c
  localeutil.c
  lock.c
  log.c
  nlist.c
  osnetutils.c
  osutils.c
  pathbld.c
  pathutil.c
  procutil.c
  progstate.c
  queue.c
  rafile.c
  slist.c
  sock.c
  sockh.c
  strlcat.c
  strlcpy.c
  sysvars.c
  tmutil.c
)
target_include_directories (libbdj4common
  PUBLIC "${GLIB_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4common
  ${GLIB_LDFLAGS}
)
addWinSockLibrary (libbdj4common)
# for RtlGetVersion
addWinNtdllLibrary (libbdj4common)

add_library (libbdj4 SHARED
  audiotag.c
  autosel.c
  bdj4init.c
  bdjvarsdf.c
  bdjvarsdfload.c
  dance.c
  dancesel.c
  dispsel.c
  dnctypes.c
  genre.c
  level.c
  musicdb.c
  musicq.c
  orgopt.c
  orgutil.c
  playlist.c
  rating.c
  sequence.c
  song.c
  songfilter.c
  songlist.c
  songsel.c
  songutil.c
  sortopt.c
  status.c
  tagdef.c
  templateutil.c
  webclient.c
)
target_include_directories (libbdj4
  PUBLIC "${CURL_INCLUDE_DIRS}"
)
target_link_libraries (libbdj4
  libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
if (APPLE)
  target_link_libraries (libbdj4
    "-framework Cocoa"
  )
endif()
addIntlLibrary (libbdj4)
addWinSockLibrary (libbdj4)

add_library (libbdj4pli SHARED
  pli.c
  dylib.c
)
target_link_libraries (libbdj4pli
  libbdj4common
  m
)

add_library (libbdj4vol SHARED
  volume.c
  dylib.c
)
target_link_libraries (libbdj4vol
  libbdj4common
  m
)

add_library (libbdj4ui SHARED
  osuilinuxutils.c
  osuimacutils.m
  osuiwinutils.c
  uimusicq.c
  uiplayer.c
  uisongsel.c
  uiutils.c
)
target_include_directories (libbdj4ui
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_link_libraries (libbdj4ui
  libbdj4 libbdj4common
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (libbdj4ui)
addWinSockLibrary (libbdj4ui)

# reusable objects; there's no need to compile these twice
add_library (libbdj4fileop STATIC
  fileop.c osutils.c tmutil.c
)
add_library (libbdj4string STATIC
  bdjstring.c strlcat.c strlcpy.c
)
add_library (libmongoose STATIC ${MONGOOSE_SRC})
add_library (libwebsrv STATIC websrv.c)
target_include_directories (libwebsrv
  PRIVATE "${MONGOOSE_INC_DIR}"
)
add_library (liblocatebdj3 STATIC locatebdj3.c)

add_library (libbdj4launcher STATIC
  filedata.c osnetutils.c pathbld.c pathutil.c sysvars.c
)

add_library (libplinull SHARED plinull.c)
target_link_libraries (libplinull
  libbdj4common
  m
)

add_library (libplivlc SHARED plivlc.c vlci.c)
target_include_directories (libplivlc
  PUBLIC "${LIBVLC_INCLUDE_DIR}"
)
target_link_libraries (libplivlc
  libbdj4common
  ${LIBVLC_LIBRARY}
  m
)

add_library (libvolalsa SHARED volalsa.c)
if (ALSA_FOUND)
  target_include_directories (libvolalsa
    PUBLIC "${ALSA_INCLUDE_DIR}"
  )
  target_link_libraries (libvolalsa
    ${ALSA_LDFLAGS}
    m
  )
endif ()

add_library (libvolmac SHARED volmac.m)
if (APPLE)
  target_link_libraries (libvolmac
    PUBLIC "-framework Cocoa" "-framework CoreAudio"
  )
endif()

add_library (libvolnull SHARED volnull.c)

add_library (libvolpa SHARED volpa.c)
if (PA_FOUND)
  target_include_directories (libvolpa
    PUBLIC "${PA_INCLUDE_DIRS}"
  )
  target_link_libraries (libvolpa
    ${PA_LDFLAGS}
    m
  )
endif()

add_library (libvolwin SHARED volwin.cpp)
target_compile_options (libvolwin PRIVATE -static-libstdc++)
target_link_libraries (libvolwin
  libbdj4fileop
  libbdj4string
)

add_executable (bdj4locatebdj3 bdj4locatebdj3.c)
target_link_libraries (bdj4locatebdj3
  liblocatebdj3
  libbdj4fileop
  libbdj4string
)

add_executable (bdj4info bdj4info.c)
target_link_libraries (bdj4info
  libbdj4common
)
addIntlLibrary (bdj4info)

add_executable (voltest voltest.c)
target_link_libraries (voltest
  libbdj4vol libbdj4common
  ${CMAKE_DL_LIBS}
  m
)

if (WIN32)
  # the bdj4se executable
  # for cmake, must use the .o file extension, not .res
  add_custom_command (
    OUTPUT selauncher.o
    COMMAND cp -f ${PROJECT_SOURCE_DIR}/../img/${BDJ4INSTICON} .
    COMMAND echo "id ICON ${BDJ4INSTICON}" > selauncher.rc
    COMMAND windres selauncher.rc -O coff -o selauncher.o
    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/../img/${BDJ4INSTICON}
    BYPRODUCTS selauncher.rc ${BDJ4INSTICON}
    VERBATIM
  )
endif()

add_executable (bdj4se bdj4se.c)
target_link_libraries (bdj4se
  libbdj4fileop
  libbdj4string
)
if (WIN32)
  target_sources (bdj4se PRIVATE selauncher.o)
endif()

if (WIN32)
  # the bdj4 executable
  # for cmake, must use the .o file extension, not .res
  add_custom_command (
    OUTPUT launcher.o
    COMMAND cp -f ${PROJECT_SOURCE_DIR}/../img/${BDJ4ICON} .
    COMMAND echo "id ICON ${BDJ4ICON}" > launcher.rc
    COMMAND windres launcher.rc -O coff -o launcher.o
    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/../img/${BDJ4ICON}
    BYPRODUCTS launcher.rc ${BDJ4ICON}
    VERBATIM
  )
endif()

add_executable (bdj4 bdj4.c)
target_link_libraries (bdj4
  libbdj4launcher
  libbdj4fileop
  libbdj4string
)
if (WIN32)
  target_sources (bdj4 PRIVATE launcher.o)
endif()
# for gethostname
addWinSockLibrary (bdj4)
addWinNtdllLibrary (bdj4)

# this is only used on macos
add_executable (bdj4g bdj4.c)
target_include_directories (bdj4g
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_compile_options (bdj4g PRIVATE -DBDJ4_GUI_LAUNCHER=1)
target_link_libraries (bdj4g
  libbdj4launcher
  libbdj4fileop
  libbdj4string
  ${GTK_LDFLAGS}
)
addWinSockLibrary (bdj4g)
addWinNtdllLibrary (bdj4g)

add_executable (bdj4cli bdj4cli.c)
target_link_libraries (bdj4cli
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addWinSockLibrary (bdj4cli)

add_executable (bdj4dbupdate bdj4dbupdate.c)
target_link_libraries (bdj4dbupdate
  libbdj4 libbdj4common
  m
)
if (WIN32)
  target_link_libraries (bdj4dbupdate -lregex)
endif()
addIntlLibrary (bdj4dbupdate)
addWinSockLibrary (bdj4dbupdate)

add_executable (bdj4dbtag bdj4dbtag.c)
target_link_libraries (bdj4dbtag
  libbdj4 libbdj4common
  m
)
target_compile_options (bdj4dbtag PRIVATE -pthread)
target_link_options (bdj4dbtag PRIVATE -pthread)
addIntlLibrary (bdj4dbtag)
addWinSockLibrary (bdj4dbtag)

add_executable (bdj4main bdj4main.c)
target_link_libraries (bdj4main
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addIntlLibrary (bdj4main)
addWinSockLibrary (bdj4main)

add_executable (bdj4player bdj4player.c)
target_link_libraries (bdj4player
  libbdj4pli libbdj4vol libbdj4 libbdj4common
  ${CMAKE_DL_LIBS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addIntlLibrary (bdj4player)
addWinSockLibrary (bdj4player)

add_executable(bdj4mobmq bdj4mobmq.c)
target_include_directories (bdj4mobmq
  PRIVATE "${MONGOOSE_INC_DIR}"
)
target_link_libraries (bdj4mobmq
  libwebsrv libmongoose
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addWinSockLibrary (bdj4mobmq)

add_executable(bdj4rc bdj4rc.c)
target_include_directories (bdj4rc
  PRIVATE "${MONGOOSE_INC_DIR}"
)
target_link_libraries (bdj4rc
  libwebsrv libmongoose
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  m
)
addWinSockLibrary (bdj4rc)

add_executable (bdj4marquee bdj4marquee.c)
target_link_libraries (bdj4marquee
  libbdj4ui libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4marquee)
addWinSockLibrary (bdj4marquee)

add_executable (bdj4installer bdj4installer.c)
target_link_libraries (bdj4installer
  liblocatebdj3
  libbdj4ui libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4installer)

add_executable (bdj4starterui bdj4starterui.c)
target_link_libraries (bdj4starterui
  libbdj4ui libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4starterui)
addWinSockLibrary (bdj4starterui)

add_executable (bdj4playerui bdj4playerui.c)
target_link_libraries (bdj4playerui
  libbdj4ui libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4playerui)
addWinSockLibrary (bdj4playerui)

add_executable (bdj4configui bdj4configui.c)
target_link_libraries (bdj4configui
  libbdj4ui libbdj4vol libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GTK_LDFLAGS}
  m
)
addIntlLibrary (bdj4configui)
addWinSockLibrary (bdj4configui)

add_executable (check_all
  check_all.c
  check_datafile.c
  check_filedata.c
  check_filemanip.c
  check_fileop.c
  check_ilist.c
  check_lock.c
  check_musicq.c
  check_nlist.c
  check_pathutil.c
  check_procutil.c
  check_queue.c
  check_rafile.c
  check_slist.c
  check_sock.c
  check_tmutil.c
)
target_link_libraries (check_all
  libbdj4 libbdj4common
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${CHECK_LDFLAGS}
  pthread
)
target_compile_options (check_all PRIVATE -pthread)
addIntlLibrary (check_all)
addWinSockLibrary (check_all)

add_executable (chkprocess chkprocess.c)

install (TARGETS
  libbdj4common libbdj4 libbdj4pli libbdj4vol libbdj4ui
  libplinull libplivlc
  libvolalsa libvolmac libvolnull libvolpa libvolwin voltest
  bdj4 bdj4g bdj4cli
  bdj4se bdj4installer
  bdj4locatebdj3 bdj4info
  bdj4dbupdate bdj4dbtag
  bdj4main bdj4player bdj4mobmq bdj4rc bdj4marquee
  bdj4starterui bdj4playerui bdj4configui
  check_all chkprocess
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

macro (macUpdateRPath name)
  add_custom_command(TARGET ${name}
      POST_BUILD
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4common.dylib"
            "@executable_path/libbdj4common.dylib"
            $<TARGET_FILE:${name}>
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4.dylib"
            "@executable_path/libbdj4.dylib"
            $<TARGET_FILE:${name}>
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4pli.dylib"
            "@executable_path/libbdj4pli.dylib"
            $<TARGET_FILE:${name}>
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4vol.dylib"
            "@executable_path/libbdj4vol.dylib"
            $<TARGET_FILE:${name}>
      COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -change
            "@rpath/libbdj4ui.dylib"
            "@executable_path/libbdj4ui.dylib"
            $<TARGET_FILE:${name}>
      VERBATIM
  )
endmacro()

if (APPLE)
  macUpdateRPath (bdj4)
  macUpdateRPath (bdj4g)
  macUpdateRPath (bdj4dbupdate)
  macUpdateRPath (bdj4main)
  macUpdateRPath (bdj4marquee)
  macUpdateRPath (bdj4mobmq)
  macUpdateRPath (bdj4player)
  macUpdateRPath (bdj4playerui)
  macUpdateRPath (bdj4rc)
  macUpdateRPath (bdj4cli)
  macUpdateRPath (bdj4installer)
  macUpdateRPath (bdj4locatebdj3)
  macUpdateRPath (bdj4locale)
  macUpdateRPath (bdj4info)
  macUpdateRPath (check_all)
endif()
