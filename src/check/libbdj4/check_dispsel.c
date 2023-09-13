/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "mdebug.h"
#include "dispsel.h"
#include "log.h"
#include "slist.h"
#include "templateutil.h"

static void
setup (void)
{
  templateDisplaySettingsCopy ();
}

START_TEST(dispsel_alloc)
{
  dispsel_t   *dsel;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dispsel_alloc");
  mdebugSubTag ("dispsel_alloc");

  dsel = dispselAlloc (DISP_SEL_LOAD_ALL);
  dispselFree (dsel);
}
END_TEST

START_TEST(dispsel_get_list)
{
  dispsel_t   *dsel;
  slist_t     *tlist;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dispsel_get_list");
  mdebugSubTag ("dispsel_get_list");


  dsel = dispselAlloc (DISP_SEL_LOAD_ALL);
  tlist = dispselGetList (dsel, DISP_SEL_MUSICQ);
  ck_assert_int_gt (slistGetCount (tlist), 0);
  tlist = dispselGetList (dsel, DISP_SEL_SONGEDIT_A);
  ck_assert_int_gt (slistGetCount (tlist), 0);
  tlist = dispselGetList (dsel, DISP_SEL_SONGSEL);
  ck_assert_int_gt (slistGetCount (tlist), 0);
  tlist = dispselGetList (dsel, DISP_SEL_SBS_SONGLIST);
  ck_assert_int_gt (slistGetCount (tlist), 0);
  dispselFree (dsel);
}
END_TEST

START_TEST(dispsel_save)
{
  dispsel_t   *dsel;
  slist_t     *tlist;
  slist_t     *tlistb;
  slistidx_t  iteridx;
  slistidx_t  iteridxb;
  const char  *vala, *valb;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dispsel_save");
  mdebugSubTag ("dispsel_save");

  dsel = dispselAlloc (DISP_SEL_LOAD_ALL);
  tlist = dispselGetList (dsel, DISP_SEL_MUSICQ);
  ck_assert_int_gt (slistGetCount (tlist), 0);

  tlistb = slistAlloc ("chk-dispsel-orig", LIST_UNORDERED, NULL);
  slistStartIterator (tlist, &iteridx);
  while ((vala = slistIterateKey (tlist, &iteridx)) != NULL) {
    slistSetNum (tlistb, vala, 0);
  }

  tlist = slistAlloc ("chk-dispsel-mq", LIST_UNORDERED, NULL);
  slistSetNum (tlist, "DANCE", 0);
  slistSetNum (tlist, "TITLE", 0);
  slistSetNum (tlist, "ARTIST", 0);
  dispselSave (dsel, DISP_SEL_MUSICQ, tlist);
  slistFree (tlist);

  /* the dispsel save process replaces the internal list */
  /* tlistb is a copy of the original */
  tlist = dispselGetList (dsel, DISP_SEL_MUSICQ);
  slistStartIterator (tlist, &iteridx);
  slistStartIterator (tlistb, &iteridxb);
  while ((vala = slistIterateKey (tlist, &iteridx)) != NULL) {
    valb = slistIterateKey (tlistb, &iteridxb);
    ck_assert_str_eq (vala, valb);
  }

  slistFree (tlistb);
  dispselFree (dsel);
}
END_TEST

Suite *
dispsel_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("dispsel");
  tc = tcase_create ("dispsel");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, dispsel_alloc);
  tcase_add_test (tc, dispsel_get_list);
  tcase_add_test (tc, dispsel_save);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
