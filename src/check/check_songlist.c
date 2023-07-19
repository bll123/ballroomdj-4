/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#if ! defined (BDJ4_MEM_DEBUG)
# define BDJ4_MEM_DEBUG
#endif

#include "bdjopt.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "filedata.h"
#include "filemanip.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "songlist.h"
#include "templateutil.h"
#include "tmutil.h"

#define SLFN "test-sl-a"
#define SLFFN "data/test-sl-a.songlist"
#define SLNEWFN "test-sl-new"
#define SLNEWFFN "data/test-sl-new.songlist"
#define SLFNB "test-sl-b"
#define SLFFNB "data/test-sl-b.songlist"

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
  filemanipCopy ("test-templates/test-songlist.songlist", "data/test-sl-a.songlist");
  filemanipCopy ("test-templates/test-songlist.pl", "data/test-sl-a.pl");
  filemanipCopy ("test-templates/test-songlist.pldances", "data/test-sl-a.pldances");
  unlink (SLNEWFFN);
}

static void
teardown (void)
{
  filemanipCopy ("test-templates/test-songlist.songlist", "data/test-sl-a.songlist");
  filemanipCopy ("test-templates/test-songlist.pl", "data/test-sl-a.pl");
  filemanipCopy ("test-templates/test-songlist.pldances", "data/test-sl-a.pldances");
  unlink (SLNEWFFN);
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

START_TEST(songlist_create)
{
  songlist_t    *sl;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songlist_create");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  sl = songlistCreate (SLNEWFN);
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
    const char    *stra;

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

/* double-free bug 2023-7-14 */
START_TEST(songlist_clear)
{
  songlist_t    *sl = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songlist_clear");

  mdebugInit ("chk-sl-bug");
  mdebugSetNoOutput ();

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  sl = songlistLoad (SLFN);
  ck_assert_ptr_nonnull (sl);
  ck_assert_int_eq (mdebugErrors (), 0);
  ck_assert_int_ne (songlistGetCount (sl), 0);
  songlistClear (sl);
  ck_assert_int_eq (mdebugErrors (), 0);
  ck_assert_int_eq (songlistGetCount (sl), 0);
  songlistFree (sl);
  ck_assert_int_eq (mdebugErrors (), 0);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
  mdebugCleanup ();
}
END_TEST

START_TEST(songlist_save)
{
  songlist_t    *sl;
  songlist_t    *slb;
  ilistidx_t    iteridx;
  ilistidx_t    key;
  time_t        tma, tmb;
  int           rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songlist_save");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  rc = songlistExists (SLFN);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (SLFFN);
  ck_assert_int_ne (rc, 0);
  sl = songlistLoad (SLFN);
  ck_assert_ptr_nonnull (sl);
  tma = fileopModTime (SLFFN);

  songlistSave (sl, SONGLIST_PRESERVE_TIMESTAMP, SONGLIST_USE_DIST_VERSION);
  tmb = fileopModTime (SLFFN);
  ck_assert_int_eq (tma, tmb);

  /* the timestamp has a granularity of one second */
  mssleep (1000);

  /* make a change to the first song list */
  songlistSetStr (sl, 0, SONGLIST_TITLE, "test-save");

  songlistSave (sl, SONGLIST_UPDATE_TIMESTAMP, SONGLIST_USE_DIST_VERSION);
  rc = songlistExists (SLFN);
  ck_assert_int_ne (rc, 0);
  tmb = fileopModTime (SLFFN);
  ck_assert_int_ne (tma, tmb);

  /* re-load the saved songlist, as the key values may have changed */
  songlistFree (sl);
  sl = songlistLoad (SLFN);
  slb = songlistLoad (SLFN);
  ck_assert_ptr_nonnull (slb);
  ck_assert_int_eq (songlistDistVersion (sl), songlistDistVersion (slb));

  /* make the same change to the second song list */
  songlistSetStr (slb, 0, SONGLIST_TITLE, "test-save");

  songlistStartIterator (sl, &iteridx);
  while ((key = songlistIterate (sl, &iteridx)) >= 0) {
    int         vala, valb;
    const char  *stra, *strb;

    vala = songlistGetNum (sl, key, SONGLIST_DANCE);
    valb = songlistGetNum (slb, key, SONGLIST_DANCE);
    ck_assert_int_eq (vala, valb);
    stra = songlistGetStr (sl, key, SONGLIST_FILE);
    strb = songlistGetStr (slb, key, SONGLIST_FILE);
    ck_assert_str_eq (stra, strb);
    stra = songlistGetStr (sl, key, SONGLIST_TITLE);
    strb = songlistGetStr (slb, key, SONGLIST_TITLE);
    ck_assert_str_eq (stra, strb);
  }
  songlistFree (sl);
  songlistFree (slb);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(songlist_save_new)
{
  songlist_t    *sl;
  songlist_t    *slb;
  ilistidx_t    iteridx;
  ilistidx_t    iteridxb;
  ilistidx_t    key;
  int           rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songlist_save_new");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  /* setup, teardown remove SLNEWFN */
  rc = songlistExists (SLNEWFN);
  ck_assert_int_eq (rc, 0);

  sl = songlistLoad (SLNEWFN);
  ck_assert_ptr_null (sl);

  sl = songlistCreate (SLNEWFN);
  ck_assert_ptr_nonnull (sl);

  rc = songlistExists (SLFN);
  ck_assert_int_ne (rc, 0);
  slb = songlistLoad (SLFN);
  ck_assert_ptr_nonnull (slb);

  songlistStartIterator (slb, &iteridxb);
  while ((key = songlistIterate (slb, &iteridxb)) >= 0) {
    int         vala;
    const char  *stra;

    vala = songlistGetNum (slb, key, SONGLIST_DANCE);
    songlistSetNum (sl, key, SONGLIST_DANCE, vala);
    stra = songlistGetStr (slb, key, SONGLIST_FILE);
    songlistSetStr (sl, key, SONGLIST_FILE, stra);
    stra = songlistGetStr (slb, key, SONGLIST_TITLE);
    songlistSetStr (sl, key, SONGLIST_TITLE, stra);
  }
  ck_assert_ptr_nonnull (sl);
  songlistSave (sl, SONGLIST_UPDATE_TIMESTAMP, SONGLIST_USE_DIST_VERSION);
  rc = songlistExists (SLNEWFN);
  ck_assert_int_ne (rc, 0);

  songlistFree (sl);
  sl = songlistLoad (SLNEWFN);
  ck_assert_ptr_nonnull (sl);

  songlistStartIterator (sl, &iteridx);
  while ((key = songlistIterate (sl, &iteridx)) >= 0) {
    int         vala, valb;
    const char  *stra, *strb;

    vala = songlistGetNum (sl, key, SONGLIST_DANCE);
    valb = songlistGetNum (slb, key, SONGLIST_DANCE);
    ck_assert_int_eq (vala, valb);
    stra = songlistGetStr (sl, key, SONGLIST_FILE);
    strb = songlistGetStr (slb, key, SONGLIST_FILE);
    ck_assert_str_eq (stra, strb);
    stra = songlistGetStr (sl, key, SONGLIST_TITLE);
    strb = songlistGetStr (slb, key, SONGLIST_TITLE);
    ck_assert_str_eq (stra, strb);
  }
  songlistFree (sl);
  songlistFree (slb);

  /* setup, teardown remove SLNEWFN */
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
  tc = tcase_create ("songlist-basic");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, songlist_exists);
  tcase_add_test (tc, songlist_create);
  tcase_add_test (tc, songlist_load);
  tcase_add_test (tc, songlist_iterate);
  tcase_add_test (tc, songlist_clear);
  suite_add_tcase (s, tc);

  tc = tcase_create ("songlist-save");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, songlist_save);
  tcase_add_test (tc, songlist_save_new);
  suite_add_tcase (s, tc);

  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
