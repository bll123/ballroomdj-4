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
#define SEQNEWFN "test-seq-new"
#define SLNEWFN "test-sl-new"
#define AUTONEWFN "test-auto-new"
#define SEQRENFN "test-seq-ren"
#define SLRENFN "test-sl-ren"
#define AUTORENFN "test-auto-ren"

static char *dbfn = "data/musicdb.dat";
static musicdb_t *db = NULL;

typedef struct {
  bool        new;
  const char  *basefn;
  char        testfn [BDJ4_PATH_MAX];
  char        ffn [BDJ4_PATH_MAX];
} test_fn_t;

static test_fn_t test_data [] = {
  { false, SEQFN, "", "" },
  { false, SEQFN, "", "" },
  { false, SEQFN, "", "" },
  { false, SLFN, "", "" },
  { false, SLFN, "", "" },
  { false, SLFN, "", "" },
  { false, AUTOFN, "", "" },
  { false, AUTOFN, "", "" },
  { true, SEQNEWFN, "", "" },
  { true, SEQNEWFN, "", "" },
  { true, SEQNEWFN, "", "" },
  { true, SLNEWFN, "", "" },
  { true, SLNEWFN, "", "" },
  { true, SLNEWFN, "", "" },
  { true, AUTONEWFN, "", "" },
  { true, AUTONEWFN, "", "" },
  { true, SEQRENFN, "", "" },
  { true, SEQRENFN, "", "" },
  { true, SEQRENFN, "", "" },
  { true, SLRENFN, "", "" },
  { true, SLRENFN, "", "" },
  { true, SLRENFN, "", "" },
  { true, AUTORENFN, "", "" },
  { true, AUTORENFN, "", "" },
};
enum {
  CPL_TEST_MAX = sizeof (test_data) / sizeof (test_fn_t),
};
enum {
  /* types */
  CPL_SEQ_OFFSET = 0,
  CPL_SL_OFFSET = 3,
  CPL_AUTO_OFFSET = 6,
  /* files for each type */
  CPL_PL_OFFSET = 0,
  CPL_PLD_OFFSET = 1,
  CPL_FILE_A_OFFSET = 2,    // seq: .sequence sl,podcast: .songlist
  CPL_FILE_B_OFFSET = 3,    // podcast: .podcast
  /* how many entries for each type */
  CPL_SEQ_COUNT = 3,
  CPL_SL_COUNT = 3,
  CPL_AUTO_COUNT = 2,
  /* test types */
  CPL_EXIST_OFFSET = 0,
  CPL_NEW_OFFSET = CPL_SEQ_COUNT + CPL_SL_COUNT + CPL_AUTO_COUNT,
  CPL_REN_OFFSET = 2 * (CPL_SEQ_COUNT + CPL_SL_COUNT + CPL_AUTO_COUNT),
};
static bool initialized = false;

static void
init (void)
{
  char        tbuff [BDJ4_PATH_MAX];
  int         idx = 0;
  const char  *ext = BDJ4_PLAYLIST_EXT;

  if (initialized) {
    return;
  }

  for (int i = 0; i < CPL_TEST_MAX; ++i) {
    if (idx == CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET) {
      ext = BDJ4_SEQUENCE_EXT;
    }
    if (idx == CPL_SL_OFFSET + CPL_FILE_A_OFFSET) {
      ext = BDJ4_SONGLIST_EXT;
    }
    if (idx == (CPL_SEQ_OFFSET + CPL_PL_OFFSET) ||
        idx == (CPL_SL_OFFSET + CPL_PL_OFFSET) ||
        idx == (CPL_AUTO_OFFSET + CPL_PL_OFFSET)) {
      ext = BDJ4_PLAYLIST_EXT;
    }
    if (idx == (CPL_SEQ_OFFSET + CPL_PLD_OFFSET) ||
        idx == (CPL_SL_OFFSET + CPL_PLD_OFFSET) ||
        idx == (CPL_AUTO_OFFSET + CPL_PLD_OFFSET)) {
      ext = BDJ4_PL_DANCE_EXT;
    }
    pathbldMakePath (tbuff, sizeof (tbuff), test_data [i].basefn, ext, PATHBLD_MP_DREL_DATA);
    stpecpy (test_data [i].ffn, test_data [i].ffn + BDJ4_PATH_MAX, tbuff);
    pathbldMakePath (tbuff, sizeof (tbuff), test_data [i].basefn, ext, PATHBLD_MP_DREL_TEST_TMPL);
    stpecpy (test_data [i].testfn, test_data [i].testfn + BDJ4_PATH_MAX, tbuff);
    ++idx;
    if (idx >= CPL_NEW_OFFSET) {
      idx = 0;
    }
  }

  initialized = true;
}

static void
cleanup (void)
{
  for (int i = 0; i < CPL_TEST_MAX; ++i) {
    if (test_data [i].new) {
      unlink (test_data [i].ffn);
    } else {
      filemanipCopy (test_data [i].testfn, test_data [i].ffn);
    }
  }
}

static void
copyfiles (void)
{
  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");
  filemanipCopy ("test-templates/status.txt", "data/status.txt");
  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");
}

static void
setup (void)
{
  init ();

  copyfiles ();

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_NONE);
  bdjvarsInit ();
  bdjvarsdfloadInit ();
  audiosrcInit ();
  db = dbOpen (dbfn);

  cleanup ();
}

static void
teardown (void)
{
  cleanup ();

  dbClose (db);
  audiosrcCleanup ();
  bdjvarsdfloadCleanup ();
  bdjvarsCleanup ();
  bdjoptCleanup ();

  copyfiles ();
}

/* playlistExists() checks for the existence of the .pl file */
START_TEST(playlist_exists)
{
  int           idxt;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_exists");
  mdebugSubTag ("playlist_exists");

  /* sequence */
  idxt = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  ck_assert_int_ne (playlistExists (test_data [idxt].basefn), 0);
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  ck_assert_int_eq (playlistExists (test_data [idxt].basefn), 0);

  /* song list */
  idxt = CPL_EXIST_OFFSET + CPL_SL_OFFSET + CPL_PL_OFFSET;
  ck_assert_int_ne (playlistExists (test_data [idxt].basefn), 0);
  idxt = CPL_NEW_OFFSET + CPL_SL_OFFSET + CPL_PL_OFFSET;
  ck_assert_int_eq (playlistExists (test_data [idxt].basefn), 0);

  /* auto */
  idxt = CPL_EXIST_OFFSET + CPL_AUTO_OFFSET + CPL_PL_OFFSET;
  ck_assert_int_ne (playlistExists (test_data [idxt].basefn), 0);
  idxt = CPL_NEW_OFFSET + CPL_AUTO_OFFSET + CPL_PL_OFFSET;
  ck_assert_int_eq (playlistExists (test_data [idxt].basefn), 0);

}
END_TEST

START_TEST(playlist_get_type)
{
  int           idxt;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_get_type");
  mdebugSubTag ("playlist_get_type");

  /* sequence */
  idxt = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  ck_assert_int_eq (playlistGetType (test_data [idxt].basefn), PLTYPE_SEQUENCE);

  /* song list */
  idxt = CPL_EXIST_OFFSET + CPL_SL_OFFSET + CPL_PL_OFFSET;
  ck_assert_int_eq (playlistGetType (test_data [idxt].basefn), PLTYPE_SONGLIST);

  /* auto */
  idxt = CPL_EXIST_OFFSET + CPL_AUTO_OFFSET + CPL_PL_OFFSET;
  ck_assert_int_eq (playlistGetType (test_data [idxt].basefn), PLTYPE_AUTO);

}
END_TEST

START_TEST(playlist_create_basic)
{
  playlist_t    *pl;
  int           idx, idxt;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_create_basic");
  mdebugSubTag ("playlist_create_basic");

  /* sequence */
  idx = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  filemanipCopy (test_data [idx].testfn, test_data [idxt].ffn);
  pl = playlistCreate (test_data [idxt].basefn, PLTYPE_SEQUENCE, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  playlistFree (pl);

  /* song list */
  idx = CPL_EXIST_OFFSET + CPL_SL_OFFSET + CPL_FILE_A_OFFSET;
  idxt = CPL_NEW_OFFSET + CPL_SL_OFFSET + CPL_FILE_A_OFFSET;
  filemanipCopy (test_data [idx].testfn, test_data [idxt].ffn);
  pl = playlistCreate (test_data [idxt].basefn, PLTYPE_SONGLIST, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  playlistFree (pl);

  /* auto */
  idxt = CPL_NEW_OFFSET + CPL_AUTO_OFFSET;
  pl = playlistCreate (test_data [idxt].basefn, PLTYPE_AUTO, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  playlistFree (pl);
}
END_TEST

START_TEST(playlist_load_basic)
{
  playlist_t    *pl;
  const char    *nm;
  int           idxt;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_load_basic");
  mdebugSubTag ("playlist_load_basic");

  /* sequence */
  idxt = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  nm = playlistGetName (pl);
  ck_assert_str_eq (nm, test_data [idxt].basefn);
  playlistFree (pl);

  /* song list */
  idxt = CPL_EXIST_OFFSET + CPL_SL_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  nm = playlistGetName (pl);
  ck_assert_str_eq (nm, test_data [idxt].basefn);
  playlistFree (pl);

  /* auto */
  idxt = CPL_EXIST_OFFSET + CPL_AUTO_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  nm = playlistGetName (pl);
  ck_assert_str_eq (nm, test_data [idxt].basefn);
  playlistFree (pl);

  /* doesn't exist */
  idxt = CPL_NEW_OFFSET + CPL_AUTO_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_null (pl);
}
END_TEST

START_TEST(playlist_get)
{
  playlist_t    *pl;
  int           idxt;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_get");
  mdebugSubTag ("playlist_get");

  /* sequence */
  idxt = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  ck_assert_int_eq (playlistGetConfigNum (pl, PLAYLIST_TYPE), PLTYPE_SEQUENCE);
  ck_assert_int_eq (playlistGetEditMode (pl), EDIT_FALSE);
  ck_assert_int_eq (playlistGetDanceNum (pl, 0, PLDANCE_DANCE), 0);
  ck_assert_int_eq (playlistGetDanceNum (pl, 3, PLDANCE_DANCE), 3);
  playlistFree (pl);

  /* song list */
  idxt = CPL_EXIST_OFFSET + CPL_SL_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  ck_assert_int_eq (playlistGetConfigNum (pl, PLAYLIST_TYPE), PLTYPE_SONGLIST);
  ck_assert_int_eq (playlistGetEditMode (pl), EDIT_FALSE);
  ck_assert_int_eq (playlistGetDanceNum (pl, 0, PLDANCE_DANCE), 0);
  ck_assert_int_eq (playlistGetDanceNum (pl, 3, PLDANCE_DANCE), 3);
  playlistFree (pl);

  /* auto */
  idxt = CPL_EXIST_OFFSET + CPL_AUTO_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  ck_assert_int_eq (playlistGetConfigNum (pl, PLAYLIST_TYPE), PLTYPE_AUTO);
  ck_assert_int_eq (playlistGetEditMode (pl), EDIT_FALSE);
  ck_assert_int_eq (playlistGetDanceNum (pl, 0, PLDANCE_DANCE), 0);
  ck_assert_int_eq (playlistGetDanceNum (pl, 3, PLDANCE_DANCE), 3);
  playlistFree (pl);
}
END_TEST

START_TEST(playlist_set)
{
  playlist_t    *pl;
  int           idxt;
  int           val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_set");
  mdebugSubTag ("playlist_set");

  /* sequence */
  idxt = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);

  val = playlistGetEditMode (pl);
  ck_assert_int_eq (val, EDIT_FALSE);
  playlistSetEditMode (pl, EDIT_TRUE);
  val = playlistGetEditMode (pl);
  ck_assert_int_eq (val, EDIT_TRUE);
  playlistSetEditMode (pl, EDIT_FALSE);

  playlistSetConfigNum (pl, PLAYLIST_LEVEL_HIGH, 1);
  val = playlistGetConfigNum (pl, PLAYLIST_LEVEL_HIGH);
  ck_assert_int_eq (val, 1);
  playlistSetConfigNum (pl, PLAYLIST_LEVEL_HIGH, 2);
  val = playlistGetConfigNum (pl, PLAYLIST_LEVEL_HIGH);
  ck_assert_int_eq (val, 2);

  playlistSetConfigNum (pl, PLAYLIST_LEVEL_LOW, 1);
  val = playlistGetConfigNum (pl, PLAYLIST_LEVEL_LOW);
  ck_assert_int_eq (val, 1);
  playlistSetConfigNum (pl, PLAYLIST_LEVEL_LOW, 0);
  val = playlistGetConfigNum (pl, PLAYLIST_LEVEL_LOW);
  ck_assert_int_eq (val, 0);

  playlistSetConfigNum (pl, PLAYLIST_GAP, 3000);
  val = playlistGetConfigNum (pl, PLAYLIST_GAP);
  ck_assert_int_eq (val, 3000);
  playlistSetConfigNum (pl, PLAYLIST_GAP, 2000);
  val = playlistGetConfigNum (pl, PLAYLIST_GAP);
  ck_assert_int_eq (val, 2000);
  playlistSetConfigNum (pl, PLAYLIST_GAP, PL_GAP_DEFAULT);
  val = playlistGetConfigNum (pl, PLAYLIST_GAP);
  ck_assert_int_eq (val, PL_GAP_DEFAULT);

  playlistSetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME, 0);
  val = playlistGetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME);
  ck_assert_int_eq (val, 0);
  playlistSetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME, 10000);
  val = playlistGetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME);
  ck_assert_int_eq (val, 10000);

  playlistSetConfigNum (pl, PLAYLIST_RATING, 1);
  val = playlistGetConfigNum (pl, PLAYLIST_RATING);
  ck_assert_int_eq (val, 1);
  playlistSetConfigNum (pl, PLAYLIST_RATING, 2);
  val = playlistGetConfigNum (pl, PLAYLIST_RATING);
  ck_assert_int_eq (val, 2);

  playlistSetConfigNum (pl, PLAYLIST_STOP_AFTER, 0);
  val = playlistGetConfigNum (pl, PLAYLIST_STOP_AFTER);
  ck_assert_int_eq (val, 0);
  playlistSetConfigNum (pl, PLAYLIST_STOP_AFTER, 3);
  val = playlistGetConfigNum (pl, PLAYLIST_STOP_AFTER);
  ck_assert_int_eq (val, 3);

  playlistSetConfigNum (pl, PLAYLIST_STOP_TIME, 0);
  val = playlistGetConfigNum (pl, PLAYLIST_STOP_TIME);
  ck_assert_int_eq (val, 0);
  playlistSetConfigNum (pl, PLAYLIST_STOP_TIME, 11 * 3600);
  val = playlistGetConfigNum (pl, PLAYLIST_STOP_TIME);
  ck_assert_int_eq (val, 11 * 3600);

  playlistSetConfigNum (pl, PLAYLIST_ANNOUNCE, true);
  val = playlistGetConfigNum (pl, PLAYLIST_ANNOUNCE);
  ck_assert_int_eq (val, true);
  playlistSetConfigNum (pl, PLAYLIST_ANNOUNCE, false);
  val = playlistGetConfigNum (pl, PLAYLIST_ANNOUNCE);
  ck_assert_int_eq (val, false);

  /* playlist_type may be changed (4.15.0) */
  val = playlistGetConfigNum (pl, PLAYLIST_TYPE);
  ck_assert_int_eq (val, PLTYPE_SEQUENCE);
  playlistSetConfigNum (pl, PLAYLIST_TYPE, PLTYPE_SONGLIST);
  val = playlistGetConfigNum (pl, PLAYLIST_TYPE);
  ck_assert_int_eq (val, PLTYPE_SONGLIST);

  playlistSetDanceNum (pl, 3, PLDANCE_MPM_HIGH, 220);
  val = playlistGetDanceNum (pl, 3, PLDANCE_MPM_HIGH);
  ck_assert_int_eq (val, 220);
  playlistSetDanceNum (pl, 3, PLDANCE_MPM_HIGH, 200);
  val = playlistGetDanceNum (pl, 3, PLDANCE_MPM_HIGH);
  ck_assert_int_eq (val, 200);

  playlistSetDanceNum (pl, 3, PLDANCE_MPM_LOW, 120);
  val = playlistGetDanceNum (pl, 3, PLDANCE_MPM_LOW);
  ck_assert_int_eq (val, 120);
  playlistSetDanceNum (pl, 3, PLDANCE_MPM_LOW, 100);
  val = playlistGetDanceNum (pl, 3, PLDANCE_MPM_LOW);
  ck_assert_int_eq (val, 100);

  playlistSetDanceNum (pl, 3, PLDANCE_COUNT, 5);
  val = playlistGetDanceNum (pl, 3, PLDANCE_COUNT);
  ck_assert_int_eq (val, 5);
  playlistSetDanceNum (pl, 3, PLDANCE_COUNT, 10);
  val = playlistGetDanceNum (pl, 3, PLDANCE_COUNT);
  ck_assert_int_eq (val, 10);

  playlistSetDanceNum (pl, 3, PLDANCE_MAXPLAYTIME, 0);
  val = playlistGetDanceNum (pl, 3, PLDANCE_MAXPLAYTIME);
  ck_assert_int_eq (val, 0);
  playlistSetDanceNum (pl, 3, PLDANCE_MAXPLAYTIME, 10000);
  val = playlistGetDanceNum (pl, 3, PLDANCE_MAXPLAYTIME);
  ck_assert_int_eq (val, 10000);

  /* pldance_dance may not be changed */
  val = playlistGetDanceNum (pl, 3, PLDANCE_DANCE);
  ck_assert_int_eq (val, 3);
  playlistSetDanceNum (pl, 3, PLDANCE_DANCE, 5);
  val = playlistGetDanceNum (pl, 3, PLDANCE_DANCE);
  ck_assert_int_eq (val, 3);

  playlistSetDanceNum (pl, 3, PLDANCE_SELECTED, true);
  val = playlistGetDanceNum (pl, 3, PLDANCE_SELECTED);
  ck_assert_int_eq (val, true);
  playlistSetDanceNum (pl, 3, PLDANCE_SELECTED, false);
  val = playlistGetDanceNum (pl, 3, PLDANCE_SELECTED);
  ck_assert_int_eq (val, false);

  /* using the set-dance-count changes selected also */
  playlistSetDanceNum (pl, 5, PLDANCE_SELECTED, true);
  playlistSetDanceNum (pl, 6, PLDANCE_SELECTED, false);
  playlistSetDanceCount (pl, 5, 0);
  playlistSetDanceCount (pl, 6, 7);
  val = playlistGetDanceNum (pl, 5, PLDANCE_SELECTED);
  ck_assert_int_eq (val, false);
  val = playlistGetDanceNum (pl, 5, PLDANCE_COUNT);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 6, PLDANCE_SELECTED);
  ck_assert_int_eq (val, true);
  val = playlistGetDanceNum (pl, 6, PLDANCE_COUNT);
  ck_assert_int_eq (val, 7);
  playlistFree (pl);
}
END_TEST

START_TEST(playlist_reset)
{
  playlist_t    *pl;
  int           idxt;
  int           val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_reset");
  mdebugSubTag ("playlist_reset");

  /* sequence */
  idxt = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);

  playlistSetDanceNum (pl, 0, PLDANCE_SELECTED, true);
  playlistSetDanceNum (pl, 1, PLDANCE_SELECTED, true);
  playlistSetDanceNum (pl, 2, PLDANCE_SELECTED, true);
  playlistSetDanceNum (pl, 3, PLDANCE_SELECTED, true);

  /* using the set-dance-count changes selected also */
  playlistSetDanceNum (pl, 5, PLDANCE_SELECTED, true);
  playlistSetDanceCount (pl, 6, 7);

  playlistResetAll (pl);
  val = playlistGetDanceNum (pl, 0, PLDANCE_SELECTED);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 1, PLDANCE_SELECTED);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 2, PLDANCE_SELECTED);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 3, PLDANCE_SELECTED);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 4, PLDANCE_SELECTED);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 5, PLDANCE_SELECTED);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 6, PLDANCE_SELECTED);
  ck_assert_int_eq (val, 0);

  val = playlistGetDanceNum (pl, 0, PLDANCE_COUNT);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 1, PLDANCE_COUNT);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 2, PLDANCE_COUNT);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 3, PLDANCE_COUNT);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 4, PLDANCE_COUNT);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 5, PLDANCE_COUNT);
  ck_assert_int_eq (val, 0);
  val = playlistGetDanceNum (pl, 6, PLDANCE_COUNT);
  ck_assert_int_eq (val, 0);

  playlistFree (pl);
}
END_TEST

START_TEST(playlist_save)
{
  playlist_t    *pl;
  int           idxt;
  int           val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_save");
  mdebugSubTag ("playlist_save");

  /* use an auto playlist here */
  /* sequences and song lists have their select flags and */
  /* counts modified on load */
  idxt = CPL_EXIST_OFFSET + CPL_AUTO_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  playlistSetDanceNum (pl, 3, PLDANCE_SELECTED, true);
  playlistSetDanceNum (pl, 3, PLDANCE_COUNT, 5);
  playlistSetDanceNum (pl, 4, PLDANCE_SELECTED, false);
  playlistSetDanceNum (pl, 4, PLDANCE_COUNT, 6);
  playlistSetConfigNum (pl, PLAYLIST_STOP_AFTER, 3);
  playlistSave (pl, test_data [idxt].basefn);
  playlistFree (pl);
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  val = playlistGetConfigNum (pl, PLAYLIST_STOP_AFTER);
  ck_assert_int_eq (val, 3);
  val = playlistGetDanceNum (pl, 3, PLDANCE_SELECTED);
  ck_assert_int_eq (val, true);
  val = playlistGetDanceNum (pl, 3, PLDANCE_COUNT);
  ck_assert_int_eq (val, 5);
  val = playlistGetDanceNum (pl, 4, PLDANCE_SELECTED);
  ck_assert_int_eq (val, false);
  val = playlistGetDanceNum (pl, 4, PLDANCE_COUNT);
  ck_assert_int_eq (val, 6);
  playlistFree (pl);
}
END_TEST

START_TEST(playlist_save_new)
{
  playlist_t    *pl;
  int           idx, idxt;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_save_new");
  mdebugSubTag ("playlist_save_new");

  cleanup ();

  /* sequence */
  idx = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  filemanipCopy (test_data [idx].testfn, test_data [idxt].ffn);
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  pl = playlistCreate (test_data [idxt].basefn, PLTYPE_SEQUENCE, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  playlistSave (pl, test_data [idxt].basefn);
  for (int i = idxt; i < idxt + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
  playlistFree (pl);

  /* song list */
  idx = CPL_EXIST_OFFSET + CPL_SL_OFFSET + CPL_FILE_A_OFFSET;
  idxt = CPL_NEW_OFFSET + CPL_SL_OFFSET + CPL_FILE_A_OFFSET;
  filemanipCopy (test_data [idx].testfn, test_data [idxt].ffn);
  idxt = CPL_NEW_OFFSET + CPL_SL_OFFSET + CPL_PL_OFFSET;
  pl = playlistCreate (test_data [idxt].basefn, PLTYPE_SONGLIST, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  playlistSave (pl, test_data [idxt].basefn);
  for (int i = idxt; i < idxt + CPL_SL_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
  playlistFree (pl);

  /* auto */
  idxt = CPL_NEW_OFFSET + CPL_AUTO_OFFSET + CPL_PL_OFFSET;
  pl = playlistCreate (test_data [idxt].basefn, PLTYPE_AUTO, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  playlistSave (pl, test_data [idxt].basefn);
  for (int i = idxt; i < idxt + CPL_AUTO_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
  playlistFree (pl);

}
END_TEST

START_TEST(playlist_rename)
{
  playlist_t    *pl;
  int           idx, idxt;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_rename");
  mdebugSubTag ("playlist_rename");

  cleanup ();

  /* sequence */
  idx = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  filemanipCopy (test_data [idx].testfn, test_data [idxt].ffn);
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  pl = playlistCreate (test_data [idxt].basefn, PLTYPE_SEQUENCE, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  playlistSave (pl, test_data [idxt].basefn);
  /* saved file names should exist */
  for (int i = idxt; i < idxt + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
  playlistFree (pl);

  idx = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  idxt = CPL_REN_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  playlistRename (test_data [idx].basefn, test_data [idxt].basefn);
  /* renamed file names should exist */
  for (int i = idxt; i < idxt + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
  /* old file names should not exist */
  for (int i = idx; i < idx + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_eq (fileopFileExists (test_data [i].ffn), 0);
  }

  cleanup ();
}
END_TEST

START_TEST(playlist_copy)
{
  playlist_t    *pl;
  int           idx, idxt;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_copy");
  mdebugSubTag ("playlist_copy");

  cleanup ();

  /* sequence */
  idx = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  filemanipCopy (test_data [idx].testfn, test_data [idxt].ffn);
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  pl = playlistCreate (test_data [idxt].basefn, PLTYPE_SEQUENCE, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  playlistSave (pl, test_data [idxt].basefn);
  /* saved file names should exist */
  for (int i = idxt; i < idxt + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
  playlistFree (pl);

  idx = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  idxt = CPL_REN_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  playlistCopy (test_data [idx].basefn, test_data [idxt].basefn);
  /* new file names should exist */
  for (int i = idxt; i < idxt + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
  /* old file names should exist */
  for (int i = idx; i < idx + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
}
END_TEST

START_TEST(playlist_delete)
{
  playlist_t    *pl;
  int           idx, idxt;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_delete");
  mdebugSubTag ("playlist_delete");

  cleanup ();

  /* sequence */
  idx = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  filemanipCopy (test_data [idx].testfn, test_data [idxt].ffn);
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  pl = playlistCreate (test_data [idxt].basefn, PLTYPE_SEQUENCE, NULL, NULL);
  ck_assert_ptr_nonnull (pl);
  playlistSave (pl, test_data [idxt].basefn);
  /* saved file names should exist */
  for (int i = idxt; i < idxt + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
  playlistFree (pl);

  idx = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  idxt = CPL_REN_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  playlistCopy (test_data [idx].basefn, test_data [idxt].basefn);
  playlistDelete (test_data [idx].basefn);
  /* new file names should exist */
  for (int i = idxt; i < idxt + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
  /* old file names should not exist due to delete */
  for (int i = idx; i < idx + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_eq (fileopFileExists (test_data [i].ffn), 0);
  }

  cleanup ();

}
END_TEST

START_TEST(playlist_chk_and_create)
{
  int           idx, idxt;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_chk_and_create");
  mdebugSubTag ("playlist_chk_and_create");

  cleanup ();

  /* sequence */
  idx = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_FILE_A_OFFSET;
  filemanipCopy (test_data [idx].testfn, test_data [idxt].ffn);
  idxt = CPL_NEW_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  playlistCheckAndCreate (test_data [idxt].basefn, PLTYPE_SEQUENCE);
  for (int i = idxt; i < idxt + CPL_SEQ_COUNT; ++i) {
    ck_assert_int_ne (fileopFileExists (test_data [i].ffn), 0);
  }
}
END_TEST

START_TEST(playlist_get_pl_list)
{
  slist_t     *pllist;
  int         val;
  const char  *sval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_get_pl_list");
  mdebugSubTag ("playlist_get_pl_list");

  cleanup ();

  /* includes the special queuedance playlist */
  pllist = playlistGetPlaylistNames (PL_LIST_ALL, NULL);
  val = slistGetCount (pllist);
  ck_assert_int_eq (val, 20);
  sval = slistGetStr (pllist, _("QueueDance"));
  ck_assert_str_eq (sval, _("QueueDance"));
  slistFree (pllist);

  /* does not include the special queuedance playlist */
  pllist = playlistGetPlaylistNames (PL_LIST_AUTO_SEQ, NULL);
  val = slistGetCount (pllist);
  ck_assert_int_eq (val, 11);
  sval = slistGetStr (pllist, _("QueueDance"));
  ck_assert_ptr_null (sval);
  slistFree (pllist);

  /* does not include the special queuedance playlist */
  pllist = playlistGetPlaylistNames (PL_LIST_NORMAL, NULL);
  val = slistGetCount (pllist);
  ck_assert_int_eq (val, 19);
  sval = slistGetStr (pllist, _("QueueDance"));
  ck_assert_ptr_null (sval);
  slistFree (pllist);

  pllist = playlistGetPlaylistNames (PL_LIST_SEQUENCE, NULL);
  val = slistGetCount (pllist);
  ck_assert_int_eq (val, 6);
  sval = slistGetStr (pllist, _("QueueDance"));
  ck_assert_ptr_null (sval);
  slistFree (pllist);

  /* includes the podcast songlist */
  pllist = playlistGetPlaylistNames (PL_LIST_SONGLIST, NULL);
  val = slistGetCount (pllist);
  ck_assert_int_eq (val, 8);
  slistFree (pllist);

  pllist = playlistGetPlaylistNames (PL_LIST_DIR, "data");
  val = slistGetCount (pllist);
  ck_assert_int_eq (val, 20);
  sval = slistGetStr (pllist, _("QueueDance"));
  ck_assert_ptr_nonnull (sval);
  slistFree (pllist);
}
END_TEST

START_TEST(playlist_get_next_sl)
{
  playlist_t  *pl;
  int         count;
  int         idxt;
  song_t      *song;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_get_next_sl");
  mdebugSubTag ("playlist_get_next_sl");

  cleanup ();

  idxt = CPL_EXIST_OFFSET + CPL_SL_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, NULL, NULL);
  ck_assert_ptr_nonnull (pl);

  /* get-next-song should fail nicely when no database has been set */
  song = playlistGetNextSong (pl, 0, NULL, NULL);
  ck_assert_ptr_null (song);
  playlistFree (pl);

  /* song list */
  idxt = CPL_EXIST_OFFSET + CPL_SL_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, db, NULL);
  ck_assert_ptr_nonnull (pl);

  count = 0;
  while ((song = playlistGetNextSong (pl, 0, NULL, NULL)) != NULL) {
    ++count;
  }
  ck_assert_int_eq (count, 72);

  playlistFree (pl);
}
END_TEST

START_TEST(playlist_get_next_sl_stop_after)
{
  playlist_t  *pl;
  int         count;
  int         idxt;
  song_t      *song;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_get_next_sl_stop_after");
  mdebugSubTag ("playlist_get_next_sl_stop_after");

  cleanup ();

  idxt = CPL_EXIST_OFFSET + CPL_SL_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, db, NULL);
  ck_assert_ptr_nonnull (pl);

  playlistSetConfigNum (pl, PLAYLIST_STOP_AFTER, 10);

  count = 0;
  while ((song = playlistGetNextSong (pl, 0, NULL, NULL)) != NULL) {
    ++count;
  }
  ck_assert_int_eq (count, 10);

  playlistFree (pl);
}
END_TEST

START_TEST(playlist_get_next_seq)
{
  playlist_t  *pl;
  int         count;
  int         idxt;
  song_t      *song;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_get_next_seq");
  mdebugSubTag ("playlist_get_next_seq");

  cleanup ();

  idxt = CPL_EXIST_OFFSET + CPL_SEQ_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, db, NULL);
  ck_assert_ptr_nonnull (pl);

  playlistSetConfigNum (pl, PLAYLIST_STOP_AFTER, 7);

  count = 0;
  while ((song = playlistGetNextSong (pl, 0, NULL, NULL)) != NULL) {
    ++count;
  }
  ck_assert_int_eq (count, 7);

  playlistFree (pl);
}
END_TEST

START_TEST(playlist_get_next_auto)
{
  playlist_t  *pl;
  int         count;
  int         idxt;
  song_t      *song;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- playlist_get_next_auto");
  mdebugSubTag ("playlist_get_next_auto");

  cleanup ();

  idxt = CPL_EXIST_OFFSET + CPL_AUTO_OFFSET + CPL_PL_OFFSET;
  pl = playlistLoad (test_data [idxt].basefn, db, NULL);
  ck_assert_ptr_nonnull (pl);

  playlistSetConfigNum (pl, PLAYLIST_STOP_AFTER, 8);

  count = 0;
  while ((song = playlistGetNextSong (pl, 0, NULL, NULL)) != NULL) {
    ++count;
  }
  ck_assert_int_eq (count, 8);

  playlistFree (pl);
}
END_TEST

Suite *
playlist_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("playlist");
  tc = tcase_create ("playlist");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, playlist_exists);
  tcase_add_test (tc, playlist_get_type);
  tcase_add_test (tc, playlist_create_basic);
  tcase_add_test (tc, playlist_load_basic);
  tcase_add_test (tc, playlist_get);
  tcase_add_test (tc, playlist_set);
  tcase_add_test (tc, playlist_reset);
  tcase_add_test (tc, playlist_save);
  tcase_add_test (tc, playlist_save_new);
  tcase_add_test (tc, playlist_rename);
  tcase_add_test (tc, playlist_copy);
  tcase_add_test (tc, playlist_delete);
  tcase_add_test (tc, playlist_chk_and_create);
  tcase_add_test (tc, playlist_get_pl_list);
  tcase_add_test (tc, playlist_get_next_sl);
  tcase_add_test (tc, playlist_get_next_sl_stop_after);
  tcase_add_test (tc, playlist_get_next_seq);
  tcase_add_test (tc, playlist_get_next_auto);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
