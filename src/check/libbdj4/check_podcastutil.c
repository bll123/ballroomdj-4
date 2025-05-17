/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "filedata.h"
#include "filemanip.h"
#include "fileop.h"
#include "log.h"
#include "musicdb.h"
#include "pathbld.h"
#include "playlist.h"
#include "slist.h"
#include "song.h"
#include "templateutil.h"

#define SEQFN "test-seq-a"
#define SLFN "test-sl-a"
#define AUTOFN "test-auto-a"
#define PODCASTFN "test-podcast-a"
#define SEQNEWFN "test-seq-new"
#define SLNEWFN "test-sl-new"
#define AUTONEWFN "test-auto-new"
#define PODCASTNEWFN "test-podcast-new"
#define SEQRENFN "test-seq-ren"
#define SLRENFN "test-sl-ren"
#define AUTORENFN "test-auto-ren"
#define PODCASTRENFN "test-podcast-ren"

static char *dbfn = "data/musicdb.dat";
static musicdb_t *db = NULL;

static void
copyfiles (void)
{
  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");
}

static void
setup (void)
{
  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");

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

  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");
}

START_TEST(podcastutil_apply)
{
}
END_TEST

START_TEST(podcastutil_delete)
{
}
END_TEST

Suite *
podcastutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("podcastutil");
  tc = tcase_create ("podcastutil");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, podcastutil_apply);
  tcase_add_test (tc, podcastutil_delete);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
