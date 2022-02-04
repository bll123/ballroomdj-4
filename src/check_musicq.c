#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "musicq.h"
#include "check_bdj.h"
#include "portability.h"
#include "log.h"

START_TEST(musicq_alloc_free)
{
  ssize_t       count;
  musicq_t      *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "=== musicq_alloc_free");
  q = musicqAlloc ();
  ck_assert_ptr_nonnull (q);
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 0);
  musicqFree (q);
}
END_TEST

START_TEST(musicq_push)
{
  ssize_t       count;
  musicq_t      *q;
  song_t        *song = (song_t *) &song;
  char          *plname = "none";

  logMsg (LOG_DBG, LOG_IMPORTANT, "=== musicq_push");
  q = musicqAlloc ();
  musicqPush (q, 0, song, plname);
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 1);
  musicqPush (q, 0, song, plname);
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 2);
  musicqPush (q, 0, song, plname);
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 3);
  musicqPush (q, 0, song, plname);
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 4);
  musicqPush (q, 0, song, plname);
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  for (int i = 0; i < 5; ++i) {
    int     uniqueidx;

    uniqueidx = musicqGetUniqueIdx (q, 0, i);
    ck_assert_int_eq (i, uniqueidx);
  }

  musicqFree (q);
}
END_TEST

START_TEST(musicq_push_pop)
{
  ssize_t       count;
  musicq_t      *q;
  song_t        *song = (song_t *) &song;
  char          *plname = "none";


  logMsg (LOG_DBG, LOG_IMPORTANT, "=== musicq_push_pop");
  q = musicqAlloc ();
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);  /* 0 1 2 3 4 */
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);  /* 1 2 3 4 5 */
  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);  /* 2 3 4 5 6 */
  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);  /* 3 4 5 6 7 */

  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  for (int i = 0; i < 5; ++i) {
    int     dispidx;
    int     uniqueidx;

    dispidx = musicqGetDispIdx (q, 0, i);
    ck_assert_int_eq (i + 4, dispidx);
    uniqueidx = musicqGetUniqueIdx (q, 0, i);
    ck_assert_int_eq (i + 3, uniqueidx);
  }

  musicqFree (q);
}
END_TEST

START_TEST(musicq_remove)
{
  ssize_t       count;
  musicq_t      *q;
  song_t        *song = (song_t *) &song;
  char          *plname = "none";
  int           bump = 0;


  logMsg (LOG_DBG, LOG_IMPORTANT, "=== musicq_remove");
  q = musicqAlloc ();
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);    /* 0 1 2 3 4 */
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);    /* 1 2 3 4 5 */

  musicqRemove (q, 0, 3);             /* 1 2 3 5 */

  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);      /* 2 3 5 6 */
  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);      /* 3 5 6 7 */
  musicqPush (q, 0, song, plname);      /* 3 5 6 7 8 */

  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);


  for (int i = 0; i < 5; ++i) {
    int     dispidx;
    int     uniqueidx;

    dispidx = musicqGetDispIdx (q, 0, i);
    ck_assert_int_eq (i + 4, dispidx);
    uniqueidx = musicqGetUniqueIdx (q, 0, i);
    ck_assert_int_eq (i + bump + 3, uniqueidx);
    if (uniqueidx == 3) {
      ++bump;
    }
  }

  musicqRemove (q, 0, 1); /* 3 6 7 8 */

  bump = 0;
  for (int i = 0; i < 4; ++i) {
    int     dispidx;
    int     uniqueidx;

    dispidx = musicqGetDispIdx (q, 0, i);
    ck_assert_int_eq (i + 4, dispidx);
    uniqueidx = musicqGetUniqueIdx (q, 0, i);
    ck_assert_int_eq (i + bump + 3, uniqueidx);
    if (uniqueidx == 3) {
      bump += 2;
    }
  }

  musicqRemove (q, 0, 1); /* 3 7 8 */

  bump = 0;
  for (int i = 0; i < 3; ++i) {
    int     dispidx;
    int     uniqueidx;

    dispidx = musicqGetDispIdx (q, 0, i);
    ck_assert_int_eq (i + 4, dispidx);
    uniqueidx = musicqGetUniqueIdx (q, 0, i);
    ck_assert_int_eq (i + bump + 3, uniqueidx);
    if (uniqueidx == 3) {
      bump += 3;
    }
  }

  musicqRemove (q, 0, 2); /* 3 7 */

  bump = 0;
  for (int i = 0; i < 2; ++i) {
    int     dispidx;
    int     uniqueidx;

    dispidx = musicqGetDispIdx (q, 0, i);
    ck_assert_int_eq (i + 4, dispidx);
    uniqueidx = musicqGetUniqueIdx (q, 0, i);
    ck_assert_int_eq (i + bump + 3, uniqueidx);
    if (uniqueidx == 3) {
      bump += 3;
    }
  }

  musicqRemove (q, 0, 1); /* 3 */

  for (int i = 0; i < 1; ++i) {
    int     dispidx;
    int     uniqueidx;

    dispidx = musicqGetDispIdx (q, 0, i);
    ck_assert_int_eq (i + 4, dispidx);
    uniqueidx = musicqGetUniqueIdx (q, 0, i);
    ck_assert_int_eq (i + 3, uniqueidx);
  }

  musicqFree (q);
}
END_TEST

START_TEST(musicq_clear)
{
  ssize_t       count;
  musicq_t      *q;
  song_t        *song = (song_t *) &song;
  char          *plname = "none";
  int           bump = 0;


  logMsg (LOG_DBG, LOG_IMPORTANT, "=== musicq_clear");
  q = musicqAlloc ();
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);  /* 0 1 2 3 4 ; 1 2 3 4 5 */
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);  /* 1 2 3 4 5 */
  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);  /* 2 3 4 5 6 */
  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);  /* 3 4 5 6 7 ; 4 5 6 7 8 */

  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  musicqClear (q, 0, 1); /* 3 ; 4 */

  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 1);

  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);  /* 3 8 9 10 11 ; 4 5 6 7 8 */

  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  bump = 0;
  for (int i = 0; i < 5; ++i) {
    int     dispidx;
    int     uniqueidx;

    dispidx = musicqGetDispIdx (q, 0, i);
    ck_assert_int_eq (i + 4, dispidx);
    uniqueidx = musicqGetUniqueIdx (q, 0, i);
    ck_assert_int_eq (i + bump + 3, uniqueidx);
    if (uniqueidx == 3) {
      bump += 4;
    }
  }

  musicqFree (q);
}
END_TEST

START_TEST(musicq_insert)
{
  ssize_t       count;
  musicq_t      *q;
  song_t        *song = (song_t *) &song;
  char          *plname = "none";
  ssize_t       vals [20];


  logMsg (LOG_DBG, LOG_IMPORTANT, "=== musicq_insert");
  q = musicqAlloc ();
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);    /* 0 1 2 3 4 */
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);    /* 1 2 3 4 5 */
  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);    /* 2 3 4 5 6 */
  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);    /* 3 4 5 6 7 */

  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  musicqInsert (q, 0, 2, song);       /* 3 4 8 5 6 7 */
  musicqInsert (q, 0, 1, song);       /* 3 9 4 8 5 6 7 */
  musicqInsert (q, 0, 6, song);       /* 3 9 4 8 5 6 10 7 */
  musicqInsert (q, 0, 1, song);       /* 3 11 9 4 8 5 6 10 7 */

  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 9);

  vals [0] = 3;
  vals [1] = 11;
  vals [2] = 9;
  vals [3] = 4;
  vals [4] = 8;
  vals [5] = 5;
  vals [6] = 6;
  vals [7] = 10;
  vals [8] = 7;

  for (int i = 0; i < 9; ++i) {
    int     dispidx;
    int     uniqueidx;

    dispidx = musicqGetDispIdx (q, 0, i);
    ck_assert_int_eq (i + 4, dispidx);
    uniqueidx = musicqGetUniqueIdx (q, 0, i);
    ck_assert_int_eq (vals [i], uniqueidx);
  }

  musicqFree (q);
}
END_TEST

START_TEST(musicq_move)
{
  ssize_t       count;
  musicq_t      *q;
  song_t        *song = (song_t *) &song;
  char          *plname = "none";
  int           vals [20];


  logMsg (LOG_DBG, LOG_IMPORTANT, "=== musicq_move");
  q = musicqAlloc ();
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);
  musicqPush (q, 0, song, plname);  /* 0 1 2 3 4 */
  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);  /* 1 2 3 4 5 */
  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);  /* 2 3 4 5 6 */
  musicqPop (q, 0);
  musicqPush (q, 0, song, plname);  /* 3 4 5 6 7 */

  musicqMove (q, 0, 1, 0);          /* 4 3 5 6 7 */
  musicqMove (q, 0, 2, 4);          /* 4 3 7 6 5 */
  musicqMove (q, 0, 3, 1);          /* 4 6 7 3 5 */

  count = musicqGetLen (q, 0);
  ck_assert_int_eq (count, 5);

  vals [0] = 4;
  vals [1] = 6;
  vals [2] = 7;
  vals [3] = 3;
  vals [4] = 5;

  for (int i = 0; i < 5; ++i) {
    int     dispidx;
    int     uniqueidx;

    dispidx = musicqGetDispIdx (q, 0, i);
    ck_assert_int_eq (i + 4, dispidx);
    uniqueidx = musicqGetUniqueIdx (q, 0, i);
    ck_assert_int_eq (vals [i], uniqueidx);
  }

  musicqFree (q);
}
END_TEST

Suite *
musicq_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Queue Suite");
  tc = tcase_create ("Queue Op");
  tcase_add_test (tc, musicq_alloc_free);
  tcase_add_test (tc, musicq_push);
  tcase_add_test (tc, musicq_push_pop);
  tcase_add_test (tc, musicq_remove);
  tcase_add_test (tc, musicq_clear);
  tcase_add_test (tc, musicq_insert);
  tcase_add_test (tc, musicq_move);
  suite_add_tcase (s, tc);

  return s;
}

