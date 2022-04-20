cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

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

if (APPLE)
  macUpdateRPath (check_all)
endif()

install (TARGETS
  check_all chkprocess
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

