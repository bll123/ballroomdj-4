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

#include "bdjopt.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "filedata.h"
#include "filemanip.h"
#include "fileop.h"
#include "log.h"
#include "nlist.h"
#include "slist.h"
#include "sequence.h"
#include "templateutil.h"

#define SEQFN "test-seq-a"
#define SEQNEWFN "test-seq-new"
#define SEQNEWFFN "data/test-seq-new.sequence"

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
  unlink (SEQNEWFFN);
}

static void
teardown (void)
{
  filemanipCopy ("test-templates/test-sequence.sequence", "data/test-seq-a.sequence");
  filemanipCopy ("test-templates/test-sequence.pl", "data/test-seq-a.pl");
  filemanipCopy ("test-templates/test-sequence.pldances", "data/test-seq-a.pldances");
  unlink (SEQNEWFFN);
}

START_TEST(sequence_exists)
{
  int     rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- sequence_exists");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  rc = sequenceExists (SEQFN);
  ck_assert_int_ne (rc, 0);
  rc = sequenceExists (SEQNEWFN);
  ck_assert_int_eq (rc, 0);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(sequence_create)
{
  sequence_t    *seq;
  slist_t       *tlist;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- sequence_create");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  seq = sequenceCreate (SEQNEWFN);
  ck_assert_ptr_nonnull (seq);
  tlist = sequenceGetDanceList (seq);
  ck_assert_int_eq (slistGetCount (tlist), 0);
  sequenceFree (seq);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(sequence_load)
{
  sequence_t    *seq;
  slist_t       *tlist;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- sequence_load");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  seq = sequenceLoad (SEQFN);
  ck_assert_ptr_nonnull (seq);
  tlist = sequenceGetDanceList (seq);
  ck_assert_int_eq (slistGetCount (tlist), 4);
  sequenceFree (seq);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(sequence_iterate)
{
  sequence_t    *seq;
  nlistidx_t    iteridx;
  nlistidx_t    fkey;
  nlistidx_t    key;
  int           count;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- sequence_iterate");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  seq = sequenceLoad (SEQFN);
  ck_assert_ptr_nonnull (seq);
  sequenceStartIterator (seq, &iteridx);
  fkey = sequenceIterate (seq, &iteridx);
  ck_assert_int_ge (fkey, 0);
  count = 1;
  while ((key = sequenceIterate (seq, &iteridx)) != fkey) {
    ck_assert_int_ge (key, 0);
    ++count;
  }
  ck_assert_int_eq (count, 4);
  sequenceFree (seq);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(sequence_save)
{
  sequence_t    *seq;
  sequence_t    *seqb;
  slist_t       *tlist;
  slist_t       *tlistb;
  nlistidx_t    iteridx;
  nlistidx_t    iteridxb;
  nlistidx_t    siteridx;
  nlistidx_t    siteridxb;
  nlistidx_t    fkey;
  nlistidx_t    key;
  nlistidx_t    tkey;
  slist_t       *tslist;
  char          *stra;
  char          *strb;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- sequence_save");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  seq = sequenceLoad (SEQFN);
  ck_assert_ptr_nonnull (seq);
  tlist = sequenceGetDanceList (seq);
  ck_assert_int_eq (slistGetCount (tlist), 4);
  tslist = slistAlloc ("chk-seq-save", LIST_UNORDERED, NULL);
  slistSetStr (tslist, "Waltz", 0);
  slistSetStr (tslist, "Tango", 0);
  slistSetStr (tslist, "Foxtrot", 0);
  slistSetStr (tslist, "Quickstep", 0);
  sequenceSave (seq, tslist);
  slistFree (tslist);

  seqb = sequenceLoad (SEQFN);

  tlist = sequenceGetDanceList (seq);
  slistStartIterator (tlist, &siteridx);
  ck_assert_int_eq (slistGetCount (tlist), 4);
  tlistb = sequenceGetDanceList (seqb);
  slistStartIterator (tlistb, &siteridxb);
  ck_assert_int_eq (slistGetCount (tlistb), 4);

  sequenceStartIterator (seq, &iteridx);
  sequenceStartIterator (seqb, &iteridxb);
  fkey = sequenceIterate (seq, &iteridx);
  tkey = sequenceIterate (seqb, &iteridxb);
  ck_assert_int_eq (fkey, tkey);

  while ((key = sequenceIterate (seq, &iteridx)) != fkey) {
    tkey = sequenceIterate (seqb, &iteridxb);
    ck_assert_int_eq (key, tkey);
  }

  while ((stra = slistIterateKey (tlist, &siteridx)) != NULL) {
    strb = slistIterateKey (tlistb, &siteridxb);
    ck_assert_str_eq (stra, strb);
  }

  sequenceFree (seq);
  sequenceFree (seqb);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(sequence_save_new)
{
  sequence_t    *seq;
  sequence_t    *seqb;
  slist_t       *tlist;
  slist_t       *tlistb;
  slist_t       *tslist;
  slistidx_t    siteridx;
  slistidx_t    siteridxb;
  char          *stra;
  char          *strb;
  int           rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- sequence_save_new");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();

  seq = sequenceLoad (SEQNEWFN);
  ck_assert_ptr_null (seq);
  seq = sequenceCreate (SEQNEWFN);
  ck_assert_ptr_nonnull (seq);
  tlist = sequenceGetDanceList (seq);
  ck_assert_int_eq (slistGetCount (tlist), 0);
  tslist = slistAlloc ("chk-seq-save-new", LIST_UNORDERED, NULL);
  slistSetStr (tslist, "Waltz", 0);
  slistSetStr (tslist, "Tango", 0);
  slistSetStr (tslist, "Foxtrot", 0);
  slistSetStr (tslist, "Quickstep", 0);
  sequenceSave (seq, tslist);
  slistFree (tslist);

  rc = sequenceExists (SEQNEWFN);
  ck_assert_int_ne (rc, 0);
  seqb = sequenceLoad (SEQNEWFN);
  ck_assert_ptr_nonnull (seqb);

  slistStartIterator (tslist, &siteridx);
  tlistb = sequenceGetDanceList (seqb);
  slistStartIterator (tlistb, &siteridxb);
  ck_assert_int_eq (slistGetCount (tlistb), 4);

  while ((stra = slistIterateKey (tslist, &siteridx)) != NULL) {
    strb = slistIterateKey (tlistb, &siteridxb);
    ck_assert_str_eq (stra, strb);
  }

  sequenceFree (seq);
  sequenceFree (seqb);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

Suite *
sequence_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("sequence");
  tc = tcase_create ("sequence");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, sequence_exists);
  tcase_add_test (tc, sequence_create);
  tcase_add_test (tc, sequence_load);
  tcase_add_test (tc, sequence_iterate);
  tcase_add_test (tc, sequence_save);
  tcase_add_test (tc, sequence_save_new);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
