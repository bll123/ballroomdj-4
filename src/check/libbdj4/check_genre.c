/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "datafile.h"
#include "genre.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("genres.txt", "genres.txt");
  bdjvarsdfloadInit ();
}

static void
teardown (void)
{
  bdjvarsdfloadCleanup ();
}

START_TEST(genre_alloc)
{
  genre_t   *genre = NULL;
  slist_t   *gl = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- genre_alloc");
  mdebugSubTag ("genre_alloc");

  genre = genreAlloc ();
  ck_assert_ptr_nonnull (genre);
  gl = genreGetList (genre);
  ck_assert_ptr_nonnull (gl);
  genreFree (genre);
}
END_TEST

START_TEST(genre_iterate)
{
  genre_t     *genre = NULL;
  slist_t     *gl = NULL;
  const char  *val = NULL;
  slistidx_t  iteridx;
  int         count;
  int         key;
  int         tkey;
  int         cf;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- genre_iterate");
  mdebugSubTag ("genre_iterate");

  genre = genreAlloc ();
  gl = genreGetList (genre);
  genreStartIterator (genre, &iteridx);
  count = 0;
  while ((key = genreIterate (genre, &iteridx)) >= 0) {
    val = genreGetGenre (genre, key);
    cf = genreGetClassicalFlag (genre, key);
    ck_assert_int_ge (cf, 0);
    ck_assert_int_le (cf, 1);

    tkey = slistGetNum (gl, val);
    ck_assert_int_eq (count, key);
    ck_assert_int_eq (count, tkey);
    ++count;
  }
  genreFree (genre);
}
END_TEST

START_TEST(genre_conv)
{
  genre_t     *genre = NULL;
  const char  *val = NULL;
  slistidx_t  iteridx;
  datafileconv_t conv;
  int         count;
  int         key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- genre_conv");
  mdebugSubTag ("genre_conv");

  genre = genreAlloc ();
  genreStartIterator (genre, &iteridx);
  count = 0;
  while ((key = genreIterate (genre, &iteridx)) >= 0) {
    val = genreGetGenre (genre, key);

    conv.invt = VALUE_STR;
    conv.str = val;
    genreConv (&conv);
    ck_assert_int_eq (conv.num, count);

    conv.invt = VALUE_NUM;
    conv.num = count;
    genreConv (&conv);
    ck_assert_str_eq (conv.str, val);

    ++count;
  }
  genreFree (genre);
}
END_TEST

START_TEST(genre_save)
{
  genre_t     *genre = NULL;
  slist_t     *gl = NULL;
  const char  *val = NULL;
  ilistidx_t  giteridx;
  ilistidx_t  iiteridx;
  int         key;
  int         cf;
  ilist_t     *tlist;
  int         tkey;
  const char  *tval;
  int         tcf;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- genre_save");
  mdebugSubTag ("genre_save");


  genre = genreAlloc ();
  gl = genreGetList (genre);

  genreStartIterator (genre, &giteridx);
  tlist = ilistAlloc ("chk-genre", LIST_ORDERED);
  while ((key = genreIterate (genre, &giteridx)) >= 0) {
    val = genreGetGenre (genre, key);
    cf = genreGetClassicalFlag (genre, key);
    ilistSetStr (tlist, key, GENRE_GENRE, val);
    ilistSetNum (tlist, key, GENRE_CLASSICAL_FLAG, cf);
  }

  genreSave (genre, tlist);
  genreFree (genre);
  bdjvarsdfloadCleanup ();

  bdjvarsdfloadInit ();
  genre = genreAlloc ();

  gl = genreGetList (genre);

  ck_assert_int_eq (ilistGetCount (tlist), slistGetCount (gl));

  genreStartIterator (genre, &giteridx);
  ilistStartIterator (tlist, &iiteridx);
  while ((key = genreIterate (genre, &giteridx)) >= 0) {
    val = genreGetGenre (genre, key);
    cf = genreGetClassicalFlag (genre, key);

    tkey = ilistIterateKey (tlist, &iiteridx);
    tval = ilistGetStr (tlist, key, GENRE_GENRE);
    tcf = ilistGetNum (tlist, key, GENRE_CLASSICAL_FLAG);

    ck_assert_int_eq (key, tkey);
    ck_assert_str_eq (val, tval);
    ck_assert_int_eq (cf, tcf);
  }

  ilistFree (tlist);
  genreFree (genre);
}
END_TEST

START_TEST(genre_set)
{
  genre_t    *genre = NULL;
  const char  *sval = NULL;
  int         nval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- genre_set");
  mdebugSubTag ("genre_set");

  genre = genreAlloc ();

  sval = genreGetGenre (genre, 0);
  ck_assert_str_ne (sval, "test");
  genreSetGenre (genre, 0, "test");
  sval = genreGetGenre (genre, 0);
  ck_assert_str_eq (sval, "test");

  nval = genreGetClassicalFlag (genre, 0);
  ck_assert_int_eq (nval, 0);
  genreSetClassicalFlag (genre, 0, 1);
  nval = genreGetClassicalFlag (genre, 0);
  ck_assert_int_eq (nval, 1);

  genreFree (genre);
}
END_TEST

START_TEST(genre_dellast)
{
  genre_t    *genre = NULL;
  int         ocount, ncount;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- genre_dellast");
  mdebugSubTag ("genre_dellast");

  genre = genreAlloc ();

  ocount = genreGetCount (genre);
  genreDeleteLast (genre);
  ncount = genreGetCount (genre);
  ck_assert_int_eq (ocount - 1, ncount);

  genreFree (genre);
}
END_TEST

Suite *
genre_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("genre");
  tc = tcase_create ("genre");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, genre_alloc);
  tcase_add_test (tc, genre_iterate);
  tcase_add_test (tc, genre_conv);
  tcase_add_test (tc, genre_save);
  tcase_add_test (tc, genre_set);
  tcase_add_test (tc, genre_dellast);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
