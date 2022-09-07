#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjopt.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "dancesel.h"
#include "filemanip.h"
#include "log.h"
#include "musicdb.h"
#include "templateutil.h"

static char *dbfn = "data/musicdb.dat";
static musicdb_t  *db = NULL;

static void
setup (void)
{
  templateFileCopy ("autoselection.txt", "autoselection.txt");
  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");
  templateFileCopy ("sortopt.txt", "sortopt.txt");
  filemanipCopy ("test-templates/status.txt", "data/status.txt");
  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_NONE);
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
}

static void
teardown (void)
{
  dbClose (db);
  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}

/* autosel variables that get used by dancesel */
/* AUTOSEL_HIST_DISTANCE */
/* AUTOSEL_TAGMATCH */
/* AUTOSEL_TAGADJUST */
/* AUTOSEL_LOG_VALUE */

START_TEST(dancesel_alloc)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_alloc");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  ds = danceselAlloc (clist);
  danceselFree (ds);
  nlistFree (clist);
}
END_TEST

Suite *
dancesel_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("dancesel");
  tc = tcase_create ("dancesel");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, dancesel_alloc);
  suite_add_tcase (s, tc);
  return s;
}

