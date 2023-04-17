/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjopt.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "filedata.h"
#include "filemanip.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "songlist.h"
#include "templateutil.h"
#include "tmutil.h"

#define SLFN "test-sl-a"
#define SLFFN "data/test-sl-a.songlist"

static void
setup (void)
{
  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");
  filemanipCopy ("test-templates/status.txt", "data/status.txt");
  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");
  filemanipCopy ("test-templates/test-songlist.songlist", "data/test-songlist.songlist");
  filemanipCopy ("test-templates/test-songlist.pl", "data/test-songlist.pl");
  filemanipCopy ("test-templates/test-songlist.pldances", "data/test-songlist.pldances");
}

START_TEST(songlist_exists)
{
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songlist_exists");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  rc = songlistExists (SLFN);
  ck_assert_int_ne (rc, 0);
  rc = songlistExists (SLFN "xyzzy");
  ck_assert_int_eq (rc, 0);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(songlist_alloc)
{
  songlist_t    *sl;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songlist_alloc");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  sl = songlistAlloc (SLFN);
  ck_assert_ptr_nonnull (sl);
  songlistFree (sl);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(songlist_load)
{
  songlist_t    *sl;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songlist_load");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  sl = songlistLoad (SLFN);
  ck_assert_ptr_nonnull (sl);
  songlistFree (sl);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(songlist_iterate)
{
  songlist_t    *sl;
  ilistidx_t    iteridx;
  int           count;
  ilistidx_t    key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songlist_iterate");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  sl = songlistLoad (SLFN);
  ck_assert_ptr_nonnull (sl);
  songlistStartIterator (sl, &iteridx);
  count = 0;
  while ((key = songlistIterate (sl, &iteridx)) >= 0) {
    ilistidx_t    vala;
    char          *stra;

    vala = songlistGetNum (sl, key, SONGLIST_DANCE);
    ck_assert_int_ge (vala, 0);
    stra = songlistGetStr (sl, key, SONGLIST_FILE);
    ck_assert_ptr_nonnull (stra);
    stra = songlistGetStr (sl, key, SONGLIST_TITLE);
    ck_assert_ptr_nonnull (stra);
    ++count;
  }
  ck_assert_int_eq (count, 72);
  songlistFree (sl);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(songlist_save)
{
  songlist_t    *sl;
  songlist_t    *slb;
  ilistidx_t    iteridx;
  ilistidx_t    iteridxb;
  ilistidx_t    key;
  time_t        tma, tmb;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songlist_save");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  sl = songlistLoad (SLFN);
  ck_assert_ptr_nonnull (sl);
  tma = fileopModTime (SLFFN);
  songlistSave (sl, SONGLIST_PRESERVE_TIMESTAMP, SONGLIST_USE_DIST_VERSION);
  tmb = fileopModTime (SLFFN);
  ck_assert_int_eq (tma, tmb);
  /* the timestamp has a granularity of one second */
  mssleep (1000);
  songlistSave (sl, SONGLIST_UPDATE_TIMESTAMP, SONGLIST_USE_DIST_VERSION);
  tmb = fileopModTime (SLFFN);
  ck_assert_int_ne (tma, tmb);

  slb = songlistLoad (SLFN);
  ck_assert_ptr_nonnull (slb);
  ck_assert_int_eq (songlistDistVersion (sl), songlistDistVersion (slb));

  songlistStartIterator (sl, &iteridx);
  songlistStartIterator (slb, &iteridxb);
  while ((key = songlistIterate (sl, &iteridx)) >= 0) {
    int   vala, valb;
    char  *stra, *strb;

    vala = songlistGetNum (sl, key, SONGLIST_DANCE);
    valb = songlistGetNum (sl, key, SONGLIST_DANCE);
    ck_assert_int_eq (vala, valb);
    stra = songlistGetStr (sl, key, SONGLIST_FILE);
    strb = songlistGetStr (sl, key, SONGLIST_FILE);
    ck_assert_str_eq (stra, strb);
    stra = songlistGetStr (sl, key, SONGLIST_TITLE);
    strb = songlistGetStr (sl, key, SONGLIST_TITLE);
    ck_assert_str_eq (stra, strb);
  }
  songlistFree (sl);
  songlistFree (slb);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

Suite *
songlist_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("songlist");
  tc = tcase_create ("songlist");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, songlist_exists);
  tcase_add_test (tc, songlist_alloc);
  tcase_add_test (tc, songlist_load);
  tcase_add_test (tc, songlist_iterate);
  suite_add_tcase (s, tc);

  tc = tcase_create ("songlist-save");
  tcase_set_tags (tc, "libbdj4 slow");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, songlist_save);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
