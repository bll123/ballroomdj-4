#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "musicdb.h"
#include "templateutil.h"

static char *dbfn = "tmp/musicdb.dat";

START_TEST(musicdb_open_new)
{
  musicdb_t *db;

  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
  dbClose (db);
  bdjvarsdfloadCleanup ();
}
END_TEST


Suite *
musicdb_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("musicdb");
  tc = tcase_create ("musicdb");
  tcase_add_test (tc, musicdb_open_new);
  suite_add_tcase (s, tc);
  return s;
}
