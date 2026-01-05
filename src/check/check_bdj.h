/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <check.h>

#if CHECK_MINOR_VERSION < 11
# define ck_assert_ptr_nonnull(ptr) ck_assert_ptr_ne(ptr,NULL)
# define ck_assert_ptr_null(ptr) ck_assert_ptr_eq(ptr,NULL)
# define ck_assert_mem_eq(ptr,val,len) ck_assert_str_eq(ptr,val)
# define ck_assert_float_eq(f,val) ck_assert_int_eq((long)(f*1000.0),(long)(val*1000.0))
#endif

void check_libcommon (SRunner *sr);
void check_libbasic (SRunner *sr);
void check_libbdj4 (SRunner *sr);
void check_libaudiosrc (SRunner *sr);
void check_libwebclient (SRunner *sr);

/* libcommon */
Suite *     bdjmsg_suite (void);
Suite *     bdjstring_suite (void);
Suite *     bdjvars_suite (void);
Suite *     callback_suite (void);
Suite *     colorutils_suite (void);
Suite *     dirop_suite (void);
Suite *     filedata_suite (void);
Suite *     filemanip_suite (void);
Suite *     fileop_suite (void);
Suite *     fileop_dir_suite (void);
Suite *     fileshared_suite (void);
Suite *     mdebug_suite (void);
Suite *     osdir_suite (void);
Suite *     osdirutil_suite (void);
Suite *     osnetutils_suite (void);
Suite *     osprocess_suite (void);
Suite *     osrandom_suite (void);
Suite *     ossignal_suite (void);
Suite *     osenv_suite (void);
Suite *     osutils_suite (void);
Suite *     pathbld_suite (void);
Suite *     pathdisp_suite (void);
Suite *     pathinfo_suite (void);
Suite *     pathutil_suite (void);
Suite *     procutil_suite (void);
Suite *     queue_suite (void);
Suite *     rafile_suite (void);
Suite *     roman_suite (void);
Suite *     sock_suite (void);
Suite *     tmutil_suite (void);
Suite *     vsencdec_suite (void);

/* libbasic */
Suite *     bdjopt_suite (void);
Suite *     datafile_suite (void);
Suite *     dirlist_suite (void);
Suite *     ilist_suite (void);
Suite *     istring_suite (void);
Suite *     nlist_suite (void);
Suite *     progstate_suite (void);
Suite *     slist_suite (void);

/* libbdj4 */
Suite *     aesencdec_suite (void);
Suite *     asconf_suite (void);
Suite *     autosel_suite (void);
Suite *     bdjvarsdf_suite (void);
Suite *     bdjvarsdfload_suite (void);
Suite *     dancesel_suite (void);
Suite *     dispsel_suite (void);
Suite *     dnctypes_suite (void);
Suite *     dance_suite (void);
Suite *     genre_suite (void);
Suite *     level_suite (void);
Suite *     lock_suite (void);
Suite *     msgparse_suite (void);
Suite *     musicdb_suite (void);
Suite *     musicq_suite (void);
Suite *     orgopt_suite (void);
Suite *     orgutil_suite (void);
Suite *     rating_suite (void);
Suite *     playlist_suite (void);
Suite *     podcast_suite (void);
Suite *     samesong_suite (void);
Suite *     sequence_suite (void);
Suite *     song_suite (void);
Suite *     songfav_suite (void);
Suite *     songfilter_suite (void);
Suite *     songlist_suite (void);
Suite *     songutil_suite (void);
Suite *     sortopt_suite (void);
Suite *     status_suite (void);
Suite *     tagdef_suite (void);
Suite *     templateutil_suite (void);
Suite *     validate_suite (void);
Suite *     volreg_suite (void);

/* libaudiosrc */
Suite *     audiosrc_suite (void);

/* libwebclient */
Suite *     webclient_suite (void);
