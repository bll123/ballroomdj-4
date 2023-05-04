#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (check_all
  check_all.c
  # libcommon
  check_libcommon.c
  check_bdjmsg.c
  check_bdjstring.c
  check_bdjvars.c
  check_callback.c
  check_colorutils.c
  check_dirop.c
  check_filedata.c
  check_filemanip.c
  check_fileshared.c
  check_fileop.c
  check_lock.c
  check_mdebug.c
  check_osdir.c
  check_osnetutils.c
  check_osprocess.c
  check_osrandom.c
  check_ossignal.c
  check_osutils.c
  check_pathbld.c
  check_pathutil.c
  check_procutil.c
  check_queue.c
  check_rafile.c
  check_sock.c
  check_tmutil.c
  check_vsencdec.c
  # libbasic
  check_libbasic.c
  check_datafile.c
  check_dirlist.c
  check_ilist.c
  check_istring.c
  check_nlist.c
  check_progstate.c
  check_slist.c
  # libbdj4
  check_libbdj4.c
  check_autosel.c
  check_bdjvarsdf.c
  check_bdjvarsdfload.c
  check_dance.c
  check_dancesel.c
  check_dispsel.c
  check_dnctypes.c
  check_genre.c
  check_level.c
  check_msgparse.c
  check_musicdb.c
  check_orgopt.c
  check_orgutil.c
  check_rating.c
  check_samesong.c
  check_sequence.c
  check_song.c
  check_songfav.c
  check_songfilter.c
  check_songlist.c
  check_songutil.c
  check_sortopt.c
  check_status.c
  check_tagdef.c
  check_templateutil.c
  check_validate.c
  check_volreg.c
  check_webclient.c
)
target_link_libraries (check_all PUBLIC
  libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${CHECK_LDFLAGS}
  pthread
  z
  m
)
target_compile_options (check_all PRIVATE -pthread)
addIntlLibrary (check_all)
addWinSockLibrary (check_all)

add_executable (chkfileshared chkfileshared.c)
target_link_libraries (chkfileshared PUBLIC
  libbdj4common
  z
  m
)

add_executable (chkprocess chkprocess.c)

install (TARGETS
  check_all chkfileshared chkprocess
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

updateRPath (check_all)
updateRPath (chkfileshared)

