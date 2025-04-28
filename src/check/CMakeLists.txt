#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (check_all
  check_all.c
  # libcommon
  libcommon/check_libcommon.c
  libcommon/check_bdjmsg.c
  libcommon/check_bdjstring.c
  libcommon/check_bdjvars.c
  libcommon/check_callback.c
  libcommon/check_colorutils.c
  libcommon/check_dirop.c
  libcommon/check_filedata.c
  libcommon/check_filemanip.c
  libcommon/check_fileshared.c
  libcommon/check_fileop.c
  libcommon/check_fileop_dir.c
  libcommon/check_mdebug.c
  libcommon/check_osdir.c
  libcommon/check_osdirutil.c
  libcommon/check_osenv.c
  libcommon/check_osnetutils.c
  libcommon/check_osprocess.c
  libcommon/check_osrandom.c
  libcommon/check_ossignal.c
  libcommon/check_osutils.c
  libcommon/check_pathbld.c
  libcommon/check_pathdisp.c
  libcommon/check_pathinfo.c
  libcommon/check_pathutil.c
  libcommon/check_procutil.c
  libcommon/check_queue.c
  libcommon/check_sock.c
  libcommon/check_tmutil.c
  libcommon/check_vsencdec.c
  libcommon/check_roman.c
  # libbasic
  libbasic/check_libbasic.c
  libbasic/check_asconf.c
  libbasic/check_bdjopt.c
  libbasic/check_datafile.c
  libbasic/check_dirlist.c
  libbasic/check_ilist.c
  libbasic/check_istring.c
  libbasic/check_lock.c
  libbasic/check_nlist.c
  libbasic/check_progstate.c
  libbasic/check_rafile.c
  libbasic/check_slist.c
  # libbdj4
  libbdj4/check_libbdj4.c
  libbdj4/check_aesencdec.c
  libbdj4/check_autosel.c
  libbdj4/check_bdjvarsdf.c
  libbdj4/check_bdjvarsdfload.c
  libbdj4/check_dance.c
  libbdj4/check_dancesel.c
  libbdj4/check_dispsel.c
  libbdj4/check_dnctypes.c
  libbdj4/check_genre.c
  libbdj4/check_level.c
  libbdj4/check_msgparse.c
  libbdj4/check_musicdb.c
  libbdj4/check_musicq.c
  libbdj4/check_orgopt.c
  libbdj4/check_orgutil.c
  libbdj4/check_playlist.c
  libbdj4/check_podcast.c
  libbdj4/check_rating.c
  libbdj4/check_samesong.c
  libbdj4/check_sequence.c
  libbdj4/check_song.c
  libbdj4/check_songfav.c
  libbdj4/check_songfilter.c
  libbdj4/check_songlist.c
  libbdj4/check_songutil.c
  libbdj4/check_sortopt.c
  libbdj4/check_status.c
  libbdj4/check_tagdef.c
  libbdj4/check_templateutil.c
  libbdj4/check_validate.c
  libbdj4/check_volreg.c
  libbdj4/check_webclient.c
  # libaudiosrc
  libaudiosrc/check_libaudiosrc.c
  libaudiosrc/check_audiosrc.c
)
target_link_libraries (check_all PRIVATE
  libbdj4 libbdj4audiosrc libbdj4basic libbdj4common
  ${ICUI18N_LDFLAGS}
  ${CURL_LDFLAGS} ${OPENSSL_LDFLAGS}
  ${GCRYPT_LDFLAGS}
  ${GLIB_LDFLAGS}
  ${CHECK_LDFLAGS}
  pthread
)
target_compile_options (check_all PRIVATE -I${PROJECT_SOURCE_DIR}/check)
target_compile_options (check_all PRIVATE -pthread)
addIntlLibrary (check_all)
addWinSockLibrary (check_all)

add_executable (chkfileshared chkfileshared.c)
target_link_libraries (chkfileshared PRIVATE
  libbdj4common
)

add_executable (chkprocess chkprocess.c)

install (TARGETS
  check_all chkfileshared chkprocess
  DESTINATION ${DEST_BIN}
)

updateRPath (check_all)
updateRPath (chkfileshared)
