cmake_minimum_required(VERSION 3.18)
project(BallroomDJ4 VERSION 4.0.0)

set (default_build_type "Debug")
set (CMAKE_MODULE_PATH CmakeModules)
set (CMAKE_SHARED_LIBRARY_PREFIX "")
set (CMAKE_STATIC_LIBRARY_PREFIX "")
set (CMAKE_INSTALL_PREFIX "")

# this seems to work on linux
SET (CMAKE_SKIP_BUILD_RPATH FALSE)
SET (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET (CMAKE_INSTALL_RPATH "\${ORIGIN}")

set (SHLIB_EXT ${CMAKE_SHARED_LIBRARY_SUFFIX})
set (MONGOOSE_INC_DIR ../packages/mongoose)
set (MONGOOSE_SRC ../packages/mongoose/mongoose.c)

# add_subdirectory (check)

include (CheckCCompilerFlag)
include (CheckFunctionExists)
include (CheckIncludeFile)
include (CheckIncludeFileCXX)
include (CheckIncludeFiles)
include (CheckLibraryExists)
include (CheckLinkerFlag)
include (CheckSymbolExists)
include (CheckTypeSize)
include (CheckVariableExists)
include (FindPkgConfig)

check_c_compiler_flag (-Wextra _cflag_extra)
check_c_compiler_flag (-Wno-unused-but-set-variable _cflag_unused_set)
check_c_compiler_flag (-Wno-unused-parameter _cflag_unused_param)
check_c_compiler_flag (-Wno-unknown-pragmas _cflag_unknown_pragma)
check_c_compiler_flag (-Wno-float-equal _cflag_float_equal)
check_c_compiler_flag (-Wno-stringop-overflow _cflag_stringop_overflow)
check_c_compiler_flag (-Wno-stringop-truncation _cflag_stringop_trunc)

find_package (ALSA)
find_package (CURL)
find_package (Intl)
find_package (Iconv)
find_package (Gettext)
find_package (OpenSSL)
find_package (LIBVLC)

pkg_check_modules (GTK gtk+-3.0)
pkg_check_modules (CHECK check)
pkg_check_modules (PA libpulse)

CHECK_INCLUDE_FILE (alsa/asoundlib.h _hdr_alsa_asoundlib)
CHECK_INCLUDE_FILE (arpa/inet.h _hdr_arpa_inet)
CHECK_INCLUDE_FILE (dlfcn.h _hdr_dlfcn)
CHECK_INCLUDE_FILE (endpointvolume.h _hdr_endpointvolume)
CHECK_INCLUDE_FILE (fcntl.h _hdr_fcntl)
CHECK_INCLUDE_FILE (io.h _hdr_io)
CHECK_INCLUDE_FILE (libintl.h _hdr_libintl)
CHECK_INCLUDE_FILE (MacTypes.h _hdr_MacTypes)
CHECK_INCLUDE_FILE (math.h _hdr_math)
CHECK_INCLUDE_FILE (netdb.h _hdr_netdb)
CHECK_INCLUDE_FILE (netinet/in.h _hdr_netinet_in)
CHECK_INCLUDE_FILE (poll.h _hdr_poll)
CHECK_INCLUDE_FILE (pthread.h _hdr_pthread)
CHECK_INCLUDE_FILE (pulse/pulseaudio.h _hdr_pulse_pulseaudio)
CHECK_INCLUDE_FILE (signal.h _hdr_signal)
CHECK_INCLUDE_FILE (stdint.h _hdr_stdint)
CHECK_INCLUDE_FILE (string.h _hdr_string)
CHECK_INCLUDE_FILE (unistd.h _hdr_unistd)
CHECK_INCLUDE_FILE (windows.h _hdr_windows)

set (CMAKE_REQUIRED_INCLUDES ${LIBVLC_INCLUDE_DIRS})
CHECK_INCLUDE_FILE (vlc/vlc.h _hdr_vlc_vlc)
set (CMAKE_REQUIRED_INCLUDES "")

set (CMAKE_REQUIRED_INCLUDES ${GTK_INCLUDE_DIRS})
CHECK_INCLUDE_FILE (gtk/gtk.h _hdr_gtk_gtk)
set (CMAKE_REQUIRED_INCLUDES "")

CHECK_INCLUDE_FILE (sys/resource.h _sys_resource)
CHECK_INCLUDE_FILE (sys/select.h _sys_select)
CHECK_INCLUDE_FILE (sys/socket.h _sys_socket)
CHECK_INCLUDE_FILE (sys/stat.h _sys_stat)
CHECK_INCLUDE_FILE (sys/time.h _sys_time)
CHECK_INCLUDE_FILE (sys/utsname.h _sys_utsname)

set (CMAKE_REQUIRED_INCLUDES windows.h)
check_function_exists (CloseHandle _lib_CloseHandle)
check_function_exists (CreateFile _lib_CreateFile)
check_function_exists (CreateProcess _lib_CreateProcess)
check_function_exists (GetFullPathName _lib_GetFullPathName)
check_function_exists (GetVersionEx _lib_GetVersionEx)
check_function_exists (LoadLibrary _lib_LoadLibrary)
check_function_exists (OpenProcess _lib_OpenProcess)
check_function_exists (TerminateProcess _lib_TerminateProcess)
check_function_exists (WriteFile _lib_WriteFile)
set (CMAKE_REQUIRED_INCLUDES "")

set (CMAKE_REQUIRED_INCLUDES winsock2.h windows.h)
set (CMAKE_REQUIRED_LIBRARIES ws2_32)
check_function_exists (ioctlsocket _lib_ioctlsocket)
check_function_exists (select _lib_select)
check_function_exists (socket _lib_socket)
check_function_exists (WSACleanup _lib_WSACleanup)
check_function_exists (WSAGetLastError _lib_WSAGetLastError)
check_function_exists (WSAStartup _lib_WSAStartup)
set (CMAKE_REQUIRED_INCLUDES "")

set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_DL_LIBS}")
check_function_exists (dlopen _lib_dlopen)
set (CMAKE_REQUIRED_LIBRARIES "")

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

set (CMAKE_REQUIRED_LIBRARIES ${VLC_LIBRARY})
check_function_exists (libvlc_new _lib_libvlc_new)
check_function_exists (libvlc_audio_output_device_enum _lib_libvlc_audio_output_device_enum)
set (CMAKE_REQUIRED_LIBRARIES "")

check_symbol_exists (INVALID_SOCKET windows.h _define_INVALID_SOCKET)
check_symbol_exists (O_CLOEXEC fcntl.h _define_O_CLOEXEC)
check_symbol_exists (O_SYNC fcntl.h _define_O_SYNC)
check_symbol_exists (SIGCHLD signal.h _define_SIGCHLD)
check_symbol_exists (SIGHUP signal.h _define_SIGHUP)
check_symbol_exists (S_IRWXU sys/stat.h _define_S_IRWXU)
check_symbol_exists (SOCK_CLOEXEC sys/socket.h _define_SOCK_CLOEXEC)
check_symbol_exists (SO_REUSEPORT sys/socket.h _define_SO_REUSEPORT)

CHECK_TYPE_SIZE (HANDLE _typ_HANDLE)
CHECK_TYPE_SIZE (SOCKET _typ_SOCKET)
CHECK_TYPE_SIZE (socklen_t _typ_socklen_t)
CHECK_TYPE_SIZE ("struct sockaddr_in" _typ_struct_sockaddr_in)
CHECK_TYPE_SIZE (suseconds_t _typ_suseconds_t)
CHECK_TYPE_SIZE (pid_t _typ_pid_t)
CHECK_TYPE_SIZE (SOCKET _typ_SOCKET)

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
  lock.c
  log.c
  nlist.c
  pathbld.c
  pathutil.c
  process.c
  progstart.c
  queue.c
  slist.c
  sock.c
  sockh.c
  songutil.c
  strlcat.c
  strlcpy.c
  sysvars.c
  tmutil.c
)
target_include_directories (libbdj4common
  PUBLIC "${CMAKE_BINARY_DIR}"
  PUBLIC "${PROJECT_SOURCE_DIR}"
)

add_library (libbdj4 SHARED
  autosel.c
  bdj4init.c
  bdjvarsdf.c
  bdjvarsdfload.c
  dance.c
  dancesel.c
  dnctypes.c
  genre.c
  level.c
  musicdb.c
  musicq.c
  playlist.c
  portability.c
  rafile.c
  rating.c
  sequence.c
  song.c
  songlist.c
  songsel.c
  sortopt.c
  status.c
  tagdef.c
  webclient.c
)
target_include_directories (libbdj4
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
  PUBLIC "${CURL_INCLUDE_DIR}"
)
target_link_libraries (libbdj4
  libbdj4common ${CURL_LIBRARIES} ${OPENSSL_SSL_LIBRARIES}
  m
)

add_library (libbdj4player SHARED
  pli.c
  volume.c
)
target_include_directories (libbdj4player
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)
target_link_libraries (libbdj4player
  libbdj4common
  m
)

add_library (libbdj4ui SHARED
  uimusicq.c
  uiplayer.c
  uisongsel.c
  uiutils.c
)
target_include_directories (libbdj4ui
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_link_libraries (libbdj4ui
  libbdj4 libbdj4common
  ${GTK_LIBRARIES}
  m
)

add_library (libplinull SHARED plinull.c)
target_include_directories (libplinull
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)

add_library (libplivlc SHARED plivlc.c vlci.c)
target_include_directories (libplivlc
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
  "${LIBVLC_INCLUDE_DIR}"
)
target_link_libraries (libplivlc
  libbdj4common
  ${LIBVLC_LIBRARY}
  m
)

add_library (libvolalsa SHARED volalsa.c)
target_include_directories (libvolalsa
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
  PUBLIC "${ALSA_INCLUDE_DIR}"
)
target_link_libraries (libvolalsa
  ${ALSA_LIBRARIES}
  m
)

add_library (libvolmac SHARED volmac.m)
target_include_directories (libvolmac
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)

add_library (libvolnull SHARED volnull.c)
target_include_directories (libvolnull
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)

add_library (libvolpa SHARED volpa.c)
target_include_directories (libvolpa
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
  PUBLIC "${PA_INCLUDE_DIRS}"
)
target_link_libraries (libvolpa
  ${PA_LIBRARIES}
  m
)

add_library (libvolwin SHARED volwin.cpp)
target_include_directories (libvolwin
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)

add_executable (bdj4 bdj4.c
  bdjstring.c filedata.c fileop.c pathbld.c
  pathutil.c sysvars.c strlcat.c strlcpy.c
)
target_include_directories (bdj4
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)

add_executable (bdj4main bdj4main.c)
target_include_directories (bdj4main
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)
target_link_libraries (bdj4main
  libbdj4 libbdj4common
  ${CURL_LIBRARIES} ${OPENSSL_SSL_LIBRARIES}
  m
)

add_executable (bdj4player bdj4player.c dylib.c)
target_include_directories (bdj4player
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)
target_link_libraries (bdj4player
  libbdj4 libbdj4common libbdj4player
  ${CMAKE_DL_LIBS}
  ${CURL_LIBRARIES} ${OPENSSL_SSL_LIBRARIES}
  m
)

add_executable(bdj4mobmq bdj4mobmq.c websrv.c ${MONGOOSE_SRC})
target_include_directories (bdj4mobmq
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
  "${MONGOOSE_INC_DIR}"
)
target_link_libraries (bdj4mobmq
  libbdj4 libbdj4common
  ${CURL_LIBRARIES} ${OPENSSL_SSL_LIBRARIES}
  m
)

add_executable(bdj4rc bdj4rc.c websrv.c ${MONGOOSE_SRC})
target_include_directories (bdj4rc
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
  "${MONGOOSE_INC_DIR}"
)
target_link_libraries (bdj4rc
  libbdj4 libbdj4common
  ${CURL_LIBRARIES} ${OPENSSL_SSL_LIBRARIES}
  m
)

add_executable (bdj4marquee bdj4marquee.c)
target_include_directories (bdj4marquee
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)
target_link_libraries (bdj4marquee
  libbdj4 libbdj4common libbdj4ui
  ${CURL_LIBRARIES} ${OPENSSL_SSL_LIBRARIES}
  ${GTK_LIBRARIES}
  m
)

add_executable (bdj4playerui bdj4playerui.c)
target_include_directories (bdj4playerui
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)
target_link_libraries (bdj4playerui
  libbdj4 libbdj4common libbdj4ui
  ${CURL_LIBRARIES} ${OPENSSL_SSL_LIBRARIES}
  ${GTK_LIBRARIES}
  m
)

#add_executable (check_all
#  check_all.c        check_list.c      check_queue.c
#  check_datafile.c   check_lock.c      check_rafile.c
#  check_filedata.c   check_musicq.c    check_slist.c
#  check_filemanip.c  check_nlist.c     check_sock.c
#  check_fileop.c     check_pathutil.c  check_tmutil.c
#  check_ilist.c      check_process.c
#)
#target_link_libraries (check_all
#  libbdj4 libbdj4common
#  ${CURL_LIBRARIES} ${OPENSSL_SSL_LIBRARIES}
#  ${CHECK_LIBRARIES}
#)

add_executable (chkprocess chkprocess.c)
target_include_directories (chkprocess
  PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
)

install (TARGETS
  libbdj4common libbdj4 libbdj4player libbdj4ui
  libplinull libplivlc
  libvolalsa libvolmac libvolnull libvolpa libvolwin
  bdj4 bdj4main bdj4player bdj4mobmq bdj4rc
  bdj4marquee bdj4playerui
  chkprocess
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
#  check_all
