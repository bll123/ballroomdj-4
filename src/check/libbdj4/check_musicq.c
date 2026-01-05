/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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

#include "audiosrc.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"
#include "musicdb.h"
#include "musicq.h"

static char *dbfn = "data/musicdb.dat";
static musicdb_t *db = NULL;

static void
setup (void)
{
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_NONE);
  bdjvarsInit ();
  bdjvarsdfloadInit ();
  audiosrcInit ();
  db = dbOpen (dbfn);
}

static void
teardown (void)
{
  dbClose (db);
  audiosrcCleanup ();
  bdjvarsdfloadCleanup ();
  bdjvarsCleanup ();
  bdjoptCleanup ();
}

START_TEST(musicq_alloc)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_alloc");
  musicq = musicqAlloc (db);
  ck_assert_ptr_nonnull (musicq);
  musicqSetDatabase (musicq, db);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 0);
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_push)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_push");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, 1, 10000);
    ck_assert_int_eq (musicqGetLen (musicq, mqidx), i + 1);
  }
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 5);
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_push_pop)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_push_pop");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, 2, 10000);
  }
  for (int i = 0; i < 5; ++i) {
    musicqPop (musicq, MUSICQ_PB_A);
    ck_assert_int_eq (musicqGetLen (musicq, mqidx), 5 - i - 1);
  }
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 0);
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_push_head)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_push_head");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, 3, 10000);
  }
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 5);
  musicqPushHeadEmpty (musicq, mqidx);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 6);
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_get_current)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;
  dbidx_t   dbidx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_get_current");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, 4, 10000);
  }
  dbidx = musicqGetCurrent (musicq, mqidx);
  ck_assert_int_eq (dbidx, 0);
  musicqPushHeadEmpty (musicq, mqidx);
  dbidx = musicqGetCurrent (musicq, mqidx);
  ck_assert_int_eq (dbidx, -1);

  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_get_by_idx)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;
  dbidx_t   dbidx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_get_by_idx");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, 4, 10000);
  }
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    dbidx = musicqGetByIdx (musicq, mqidx, i);
    ck_assert_int_eq (dbidx, i);
  }

  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_insert)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;
  dbidx_t   dbidx;
  dbidx_t   tdbidx [] = { 0, 7, 1, 6, 2, 3, 8, 4, 9, };
  int       val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_insert");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, 4, 10000);
  }
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 5);
  /* insert in the middle */
  val = musicqInsert (musicq, mqidx, 2, 6, 10000);
  ck_assert_int_eq (val, 2);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 6);
  /* insert at the beginning */
  val = musicqInsert (musicq, mqidx, 1, 7, 10000);
  ck_assert_int_eq (val, 1);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 7);
  /* insert at the end */
  val = musicqInsert (musicq, mqidx, 6, 8, 10000);
  ck_assert_int_eq (val, 6);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 8);
  /* insert at the very end */
  val = musicqInsert (musicq, mqidx, 999, 9, 10000);
  ck_assert_int_eq (val, 8);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 9);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    dbidx = musicqGetByIdx (musicq, mqidx, i);
    ck_assert_int_eq (dbidx, tdbidx [i]);
  }
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_move)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;
  dbidx_t   dbidx;
  dbidx_t   tdbidx [] = { 1, 0, 2, 3, 6, 5, 4, };

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_move");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 7; ++i) {
    musicqPush (musicq, mqidx, i, 6, 10000);
  }
  /* move is usually used for next/previous */
  musicqMove (musicq, mqidx, 0, 1);
  musicqMove (musicq, mqidx, 4, 5);
  musicqMove (musicq, mqidx, 5, 6);
  musicqMove (musicq, mqidx, 5, 4);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 7);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    dbidx = musicqGetByIdx (musicq, mqidx, i);
    ck_assert_int_eq (dbidx, tdbidx [i]);
  }
  /* bad index */
  musicqMove (musicq, mqidx, 5, 8);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    dbidx = musicqGetByIdx (musicq, mqidx, i);
    ck_assert_int_eq (dbidx, tdbidx [i]);
  }
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_swap)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;
  dbidx_t   dbidx;
  dbidx_t   tdbidx [] = { 0, 6, 2, 1, 5, 3, 4, };

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_swap");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 7; ++i) {
    musicqPush (musicq, mqidx, i, 6, 10000);
  }
  musicqSwap (musicq, mqidx, 1, 3);
  musicqSwap (musicq, mqidx, 4, 6);
  musicqSwap (musicq, mqidx, 1, 4);
  musicqSwap (musicq, mqidx, 5, 4);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 7);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    dbidx = musicqGetByIdx (musicq, mqidx, i);
    ck_assert_int_eq (dbidx, tdbidx [i]);
  }
  /* bad index */
  musicqSwap (musicq, mqidx, 5, 12);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    dbidx = musicqGetByIdx (musicq, mqidx, i);
    ck_assert_int_eq (dbidx, tdbidx [i]);
  }
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_remove)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;
  dbidx_t   dbidx;
  dbidx_t   tdbidx [] = { 0, 2, 4, 5, };

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_remove");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 7; ++i) {
    musicqPush (musicq, mqidx, i, 6, 10000);
  }
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 7);
  musicqRemove (musicq, mqidx, 6);
  musicqRemove (musicq, mqidx, 1);
  musicqRemove (musicq, mqidx, 2);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 4);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    dbidx = musicqGetByIdx (musicq, mqidx, i);
    ck_assert_int_eq (dbidx, tdbidx [i]);
  }
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_clear)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_clear");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 7; ++i) {
    musicqPush (musicq, mqidx, i, 6, 10000);
  }
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 7);
  musicqClear (musicq, mqidx, 4);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 4);
  musicqClear (musicq, mqidx, 1);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 1);
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_get_pl_idx)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_get_pl_idx");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, i, 10000);
  }
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    ck_assert_int_eq (musicqGetPlaylistIdx (musicq, mqidx, i), i);
  }
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_get_disp_idx)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;
  int       val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_get_disp_idx");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, 5, 10000);
  }
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 5);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    ck_assert_int_eq (musicqGetDispIdx (musicq, mqidx, i), i + 1);
  }
  musicqPushHeadEmpty (musicq, mqidx);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 6);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    ck_assert_int_eq (musicqGetDispIdx (musicq, mqidx, i), i);
  }
  musicqPop (musicq, mqidx);
  musicqPop (musicq, mqidx);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 4);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    ck_assert_int_eq (musicqGetDispIdx (musicq, mqidx, i), i + 2);
  }
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, 5, 10000);
  }
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 9);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    ck_assert_int_eq (musicqGetDispIdx (musicq, mqidx, i), i + 2);
  }
  val = musicqInsert (musicq, mqidx, 2, 6, 10000);
  ck_assert_int_eq (val, 2);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 10);
  val = musicqInsert (musicq, mqidx, 1, 7, 10000);
  ck_assert_int_eq (val, 1);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 11);
  val = musicqInsert (musicq, mqidx, 7, 8, 10000);
  ck_assert_int_eq (val, 7);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 12);
  val = musicqInsert (musicq, mqidx, 5, 9, 10000);
  ck_assert_int_eq (val, 5);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 13);
  val = musicqInsert (musicq, mqidx, 2, 10, 10000);
  ck_assert_int_eq (val, 2);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 14);
  val = musicqInsert (musicq, mqidx, 999, 11, 10000);
  ck_assert_int_eq (val, 14);
  ck_assert_int_eq (musicqGetLen (musicq, mqidx), 15);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    ck_assert_int_eq (musicqGetDispIdx (musicq, mqidx, i), i + 2);
  }
  musicqMove (musicq, mqidx, 0, 1);
  musicqMove (musicq, mqidx, 4, 5);
  musicqMove (musicq, mqidx, 6, 7);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    ck_assert_int_eq (musicqGetDispIdx (musicq, mqidx, i), i + 2);
  }
  musicqSwap (musicq, mqidx, 1, 3);
  musicqSwap (musicq, mqidx, 4, 6);
  musicqSwap (musicq, mqidx, 1, 4);
  musicqSwap (musicq, mqidx, 5, 4);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    ck_assert_int_eq (musicqGetDispIdx (musicq, mqidx, i), i + 2);
  }
  musicqRemove (musicq, mqidx, 8);
  musicqRemove (musicq, mqidx, 7);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    ck_assert_int_eq (musicqGetDispIdx (musicq, mqidx, i), i + 2);
  }
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_get_unique_idx)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;
  int       tu [50];
  int       uc = 0;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_get_unique_idx");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, i, 10000);
  }
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    tu [uc++] = musicqGetUniqueIdx (musicq, mqidx, i);
  }
  for (int i = 0; i < 5; ++i) {
    musicqPop (musicq, mqidx);
  }
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    musicqPush (musicq, mqidx, i, i, 10000);
  }
  musicqInsert (musicq, mqidx, 2, 6, 10000);
  musicqInsert (musicq, mqidx, 1, 7, 10000);
  musicqInsert (musicq, mqidx, 7, 8, 10000);
  musicqInsert (musicq, mqidx, 5, 9, 10000);
  musicqInsert (musicq, mqidx, 2, 10, 10000);
  musicqInsert (musicq, mqidx, 999, 11, 10000);
  for (int i = 0; i < musicqGetLen (musicq, mqidx); ++i) {
    tu [uc++] = musicqGetUniqueIdx (musicq, mqidx, i);
  }
  for (int i = 0; i < uc; ++i) {
    for (int j = 0; j < uc; ++j) {
      if (i == j) { continue; }
      ck_assert_int_ne (tu [i], tu [j]);
    }
  }
  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_flags)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;
  int       val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_flags");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 10; ++i) {
    musicqPush (musicq, mqidx, i, i, 10000);
  }
  musicqSetFlag (musicq, mqidx, 1, MUSICQ_FLAG_PAUSE);
  musicqSetFlag (musicq, mqidx, 2, MUSICQ_FLAG_REQUEST);

  val = musicqGetFlags (musicq, mqidx, 2);
  ck_assert_int_eq (val, MUSICQ_FLAG_REQUEST);

  val = musicqGetFlags (musicq, mqidx, 1);
  ck_assert_int_eq (val, MUSICQ_FLAG_PAUSE);

  musicqSetFlag (musicq, mqidx, 1, MUSICQ_FLAG_REQUEST);
  val = musicqGetFlags (musicq, mqidx, 1);
  ck_assert_int_eq (val, MUSICQ_FLAG_PAUSE | MUSICQ_FLAG_REQUEST);

  musicqClearFlag (musicq, mqidx, 1, MUSICQ_FLAG_PAUSE);
  val = musicqGetFlags (musicq, mqidx, 1);
  ck_assert_int_eq (val, MUSICQ_FLAG_REQUEST);

  musicqFree (musicq);
}
END_TEST

START_TEST(musicq_announce)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;
  const char  *sval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_announce");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 10; ++i) {
    musicqPush (musicq, mqidx, i, i, 10000);
  }

  musicqSetAnnounce (musicq, mqidx, 1, "abc123");
  musicqSetAnnounce (musicq, mqidx, 2, "def123");
  sval = musicqGetAnnounce (musicq, mqidx, 2);
  ck_assert_str_eq (sval, "def123");
  sval = musicqGetAnnounce (musicq, mqidx, 1);
  ck_assert_str_eq (sval, "abc123");

  musicqFree (musicq);
}
END_TEST


START_TEST(musicq_duration)
{
  musicq_t  *musicq;
  int       mqidx = MUSICQ_PB_A;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicq_duration");
  musicq = musicqAlloc (db);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, 5, 10000);
  }
  ck_assert_int_eq (musicqGetDuration (musicq, mqidx),
      musicqGetLen (musicq, mqidx) * 10000);
  musicqPushHeadEmpty (musicq, mqidx);
  ck_assert_int_eq (musicqGetDuration (musicq, mqidx),
      (musicqGetLen (musicq, mqidx) - 1) * 10000);
  musicqPop (musicq, mqidx);
  musicqPop (musicq, mqidx);
  ck_assert_int_eq (musicqGetDuration (musicq, mqidx),
      musicqGetLen (musicq, mqidx) * 10000);
  for (int i = 0; i < 5; ++i) {
    musicqPush (musicq, mqidx, i, 5, 10000);
  }
  ck_assert_int_eq (musicqGetDuration (musicq, mqidx),
      musicqGetLen (musicq, mqidx) * 10000);
  musicqInsert (musicq, mqidx, 2, 6, 10000);
  musicqInsert (musicq, mqidx, 1, 7, 10000);
  musicqInsert (musicq, mqidx, 7, 8, 10000);
  musicqInsert (musicq, mqidx, 5, 9, 10000);
  musicqInsert (musicq, mqidx, 2, 10, 10000);
  musicqInsert (musicq, mqidx, 999, 11, 10000);
  ck_assert_int_eq (musicqGetDuration (musicq, mqidx),
      musicqGetLen (musicq, mqidx) * 10000);
  musicqRemove (musicq, mqidx, 8);
  musicqRemove (musicq, mqidx, 7);
  ck_assert_int_eq (musicqGetDuration (musicq, mqidx),
      musicqGetLen (musicq, mqidx) * 10000);
  musicqFree (musicq);
}
END_TEST

Suite *
musicq_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("musicq");
  tc = tcase_create ("musicq");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, musicq_alloc);
  tcase_add_test (tc, musicq_push);
  tcase_add_test (tc, musicq_push_pop);
  tcase_add_test (tc, musicq_push_head);
  tcase_add_test (tc, musicq_get_current);
  tcase_add_test (tc, musicq_get_by_idx);
  tcase_add_test (tc, musicq_insert);
  tcase_add_test (tc, musicq_move);
  tcase_add_test (tc, musicq_swap);
  tcase_add_test (tc, musicq_remove);
  tcase_add_test (tc, musicq_clear);
  tcase_add_test (tc, musicq_get_pl_idx);
  tcase_add_test (tc, musicq_get_disp_idx);
  tcase_add_test (tc, musicq_get_unique_idx);
  tcase_add_test (tc, musicq_flags);
  tcase_add_test (tc, musicq_announce);
  tcase_add_test (tc, musicq_duration);
/* get dance */
/* get data */
/* next queue */
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
