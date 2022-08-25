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
#include "datafile.h"
#include "ilist.h"
#include "status.h"
#include "slist.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("status.txt", "status.txt");
}

START_TEST(status_alloc)
{
  status_t   *status = NULL;

  status = statusAlloc ();
  ck_assert_ptr_nonnull (status);
  statusFree (status);
}
END_TEST

START_TEST(status_iterate)
{
  status_t     *status = NULL;
  char        *val = NULL;
  slistidx_t  iteridx;
  int         count;
  int         key;
  int         pf;

  status = statusAlloc ();
  statusStartIterator (status, &iteridx);
  count = 0;
  while ((key = statusIterate (status, &iteridx)) >= 0) {
    val = statusGetStatus (status, key);
    pf = statusGetPlayFlag (status, key);
    ck_assert_int_ge (pf, 0);
    ck_assert_int_le (pf, 1);
    ++count;
  }
  ck_assert_int_eq (count, statusGetCount (status));
  ck_assert_int_ge (statusGetMaxWidth (status), 1);
  statusFree (status);
}
END_TEST

START_TEST(status_conv)
{
  status_t     *status = NULL;
  char        *val = NULL;
  slistidx_t  iteridx;
  datafileconv_t conv;
  int         count;
  int         key;

  bdjvarsdfloadInit ();

  status = statusAlloc ();
  statusStartIterator (status, &iteridx);
  count = 0;
  while ((key = statusIterate (status, &iteridx)) >= 0) {
    val = statusGetStatus (status, key);

    conv.allocated = false;
    conv.valuetype = VALUE_STR;
    conv.str = val;
    statusConv (&conv);
    ck_assert_int_eq (conv.num, count);

    conv.allocated = false;
    conv.valuetype = VALUE_NUM;
    conv.num = count;
    statusConv (&conv);
    ck_assert_str_eq (conv.str, val);
    if (conv.allocated) {
      free (conv.str);
    }

    ++count;
  }
  statusFree (status);
  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(status_save)
{
  status_t     *status = NULL;
  char        *val = NULL;
  ilistidx_t  giteridx;
  ilistidx_t  iiteridx;
  int         key;
  int         pf;
  ilist_t     *tlist;
  int         tkey;
  char        *tval;
  int         tpf;

  /* required for the status conversion function */
  bdjvarsdfloadInit ();

  status = statusAlloc ();

  statusStartIterator (status, &giteridx);
  tlist = ilistAlloc ("chk-status", LIST_ORDERED);
  while ((key = statusIterate (status, &giteridx)) >= 0) {
    val = statusGetStatus (status, key);
    pf = statusGetPlayFlag (status, key);
    ilistSetStr (tlist, key, STATUS_STATUS, val);
    ilistSetNum (tlist, key, STATUS_PLAY_FLAG, pf);
  }

  statusSave (status, tlist);
  statusFree (status);
  bdjvarsdfloadCleanup ();

  bdjvarsdfloadInit ();
  status = statusAlloc ();

  ck_assert_int_eq (ilistGetCount (tlist), statusGetCount (status));

  statusStartIterator (status, &giteridx);
  ilistStartIterator (tlist, &iiteridx);
  while ((key = statusIterate (status, &giteridx)) >= 0) {
    val = statusGetStatus (status, key);
    pf = statusGetPlayFlag (status, key);

    tkey = ilistIterateKey (tlist, &iiteridx);
    tval = ilistGetStr (tlist, key, STATUS_STATUS);
    tpf = ilistGetNum (tlist, key, STATUS_PLAY_FLAG);

    ck_assert_int_eq (key, tkey);
    ck_assert_str_eq (val, tval);
    ck_assert_int_eq (pf, tpf);
  }

  ilistFree (tlist);
  statusFree (status);
  bdjvarsdfloadCleanup ();
}
END_TEST

Suite *
status_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("status");
  tc = tcase_create ("status");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, status_alloc);
  tcase_add_test (tc, status_iterate);
  tcase_add_test (tc, status_conv);
  tcase_add_test (tc, status_save);
  suite_add_tcase (s, tc);
  return s;
}
