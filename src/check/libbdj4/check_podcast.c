/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "fileop.h"
#include "mdebug.h"
#include "log.h"
#include "mdebug.h"
#include "podcast.h"
#include "nlist.h"
#include "templateutil.h"

static void
setup (void)
{
  bdjvarsdfloadInit ();
  fileopDelete ("data/tpodcast.podcast");
  fileopDelete ("data/tpodb.podcast");
}

static void
teardown (void)
{
  bdjvarsdfloadCleanup ();
  fileopDelete ("data/tpodcast.podcast");
  fileopDelete ("data/tpodb.podcast");
}

START_TEST(podcast_create)
{
  podcast_t   *podcast = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- podcast_create");
  mdebugSubTag ("podcast_create");

  podcast = podcastCreate ("tpodcast");
  ck_assert_ptr_nonnull (podcast);
  podcastFree (podcast);
}
END_TEST

START_TEST(podcast_setget)
{
  podcast_t     *podcast = NULL;
  const char    *sval;
  int32_t       nval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- podcast_setget");
  mdebugSubTag ("podcast_setget");

  podcast = podcastCreate ("tpodcast");

  podcastSetStr (podcast, PODCAST_URI, "test");
  podcastSetStr (podcast, PODCAST_TITLE, "title-a");
  podcastSetNum (podcast, PODCAST_RETAIN, 10);

  sval = podcastGetStr (podcast, PODCAST_URI);
  ck_assert_str_eq (sval, "test");
  sval = podcastGetStr (podcast, PODCAST_TITLE);
  ck_assert_str_eq (sval, "title-a");
  nval = podcastGetNum (podcast, PODCAST_RETAIN);
  ck_assert_int_eq (nval, 10);

  podcastSetStr (podcast, PODCAST_URI, "testb");
  podcastSetStr (podcast, PODCAST_TITLE, "title-b");
  podcastSetNum (podcast, PODCAST_RETAIN, 11);

  sval = podcastGetStr (podcast, PODCAST_URI);
  ck_assert_str_eq (sval, "testb");
  sval = podcastGetStr (podcast, PODCAST_TITLE);
  ck_assert_str_eq (sval, "title-b");
  nval = podcastGetNum (podcast, PODCAST_RETAIN);
  ck_assert_int_eq (nval, 11);

  podcastFree (podcast);
}
END_TEST

START_TEST(podcast_save)
{
  podcast_t     *podcast = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- podcast_save");
  mdebugSubTag ("podcast_save");

  podcast = podcastCreate ("tpodcast");

  podcastSetStr (podcast, PODCAST_URI, "test");
  podcastSetStr (podcast, PODCAST_TITLE, "title");
  podcastSetNum (podcast, PODCAST_RETAIN, 10);

  podcastSave (podcast);
  podcastFree (podcast);
}
END_TEST

START_TEST(podcast_exists)
{
  int           rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- podcast_exists");
  mdebugSubTag ("podcast_exists");

  rc = podcastExists ("tpodcast");
  ck_assert_int_eq (rc, 1);
}
END_TEST

START_TEST(podcast_load)
{
  podcast_t     *podcast = NULL;
  const char  *sval = NULL;
  int         nval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- podcast_load");
  mdebugSubTag ("podcast_load");

  podcast = podcastLoad ("tpodcast");

  sval = podcastGetStr (podcast, PODCAST_URI);
  ck_assert_str_eq (sval, "test");
  sval = podcastGetStr (podcast, PODCAST_TITLE);
  ck_assert_str_eq (sval, "title");
  nval = podcastGetNum (podcast, PODCAST_RETAIN);
  ck_assert_int_eq (nval, 10);

  podcastFree (podcast);
}
END_TEST

START_TEST(podcast_setname)
{
  podcast_t   *podcast = NULL;
  const char  *sval = NULL;
  int         nval;
  int         rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- podcast_load");
  mdebugSubTag ("podcast_load");

  podcast = podcastLoad ("tpodcast");
  podcastSetName (podcast, "tpodb");
  podcastSave (podcast);
  podcastFree (podcast);

  rc = podcastExists ("tpodcast");
  ck_assert_int_eq (rc, 1);
  rc = podcastExists ("tpodb");
  ck_assert_int_eq (rc, 1);

  podcast = podcastLoad ("tpodb");

  sval = podcastGetStr (podcast, PODCAST_URI);
  ck_assert_str_eq (sval, "test");
  sval = podcastGetStr (podcast, PODCAST_TITLE);
  ck_assert_str_eq (sval, "title");
  nval = podcastGetNum (podcast, PODCAST_RETAIN);
  ck_assert_int_eq (nval, 10);

  podcastFree (podcast);
}
END_TEST

Suite *
podcast_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("podcast");
  tc = tcase_create ("podcast");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, podcast_create);
  tcase_add_test (tc, podcast_setget);
  tcase_add_test (tc, podcast_save);
  tcase_add_test (tc, podcast_exists);
  tcase_add_test (tc, podcast_load);
  tcase_add_test (tc, podcast_setname);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
