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

#include "bdjopt.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "filedata.h"
#include "filemanip.h"
#include "fileop.h"
#include "songlist.h"
#include "templateutil.h"

#define SLFN "test-songlist"

static void
setup (void)
{
  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");
  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");
  filemanipCopy ("test-templates/test-songlist.songlist", "data/test-songlist.songlist");
  filemanipCopy ("test-templates/test-songlist.pl", "data/test-songlist.pl");
  filemanipCopy ("test-templates/test-songlist.pldances", "data/test-songlist.pldances");
}

START_TEST(songlist_alloc)
{
  songlist_t    *sl;

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
  ck_assert_int_eq (count, 78);
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

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  sl = songlistLoad (SLFN);
  ck_assert_ptr_nonnull (sl);
  songlistSave (sl);

  slb = songlistLoad (SLFN);
  ck_assert_ptr_nonnull (slb);

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
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, songlist_alloc);
  tcase_add_test (tc, songlist_load);
  tcase_add_test (tc, songlist_iterate);
  tcase_add_test (tc, songlist_save);
  suite_add_tcase (s, tc);
  return s;
}
