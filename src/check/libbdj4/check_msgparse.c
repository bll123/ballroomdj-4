/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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

#include "bdjmsg.h"
#include "bdjstring.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"
#include "msgparse.h"
#include "nlist.h"
#include "sysvars.h"

typedef struct {
  int   dbidx;
  int   didx;
  int   uidx;
  int   pind;
} titem_t;

titem_t mptestdata [] = {
  { 2, 1, 20, 0 },
  { 3, 2, 26, 0 },
  { 4, 3, 16, 0 },
  { 5, 4, 111, 1 },
  { 6, 5, 104, 0 },
  { 7, 6, 2, 0 },
  { 8, 7, 52, 0 },
  { 9, 8, 75, 0 },
};
enum {
  titemsz = sizeof (mptestdata) / sizeof (titem_t),
};


START_TEST(msgparse_mq_data)
{
  char                tbuff [4000];
  char                tmp [40];
  mp_musicqupdate_t   *mpmqu = NULL;
  nlist_t             *list;
  mp_musicqupditem_t  *mpmqitem = NULL;
  char                *p = tbuff;
  char                *end = tbuff + sizeof (tbuff);

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- msgparse_mq_data");
  mdebugSubTag ("msgparse_mq_data");

  snprintf (tbuff, sizeof (tbuff), "6%c2761800%c77%c",
      MSG_ARGS_RS, MSG_ARGS_RS, MSG_ARGS_RS);
  p = tbuff + strlen (tbuff);
  for (int i = 0; i < titemsz; ++i) {
    snprintf (tmp, sizeof (tmp), "%d%c%d%c%d%c%d%c",
      mptestdata [i].didx, MSG_ARGS_RS, mptestdata [i].uidx, MSG_ARGS_RS,
      mptestdata [i].dbidx, MSG_ARGS_RS, mptestdata [i].pind, MSG_ARGS_RS);
    p = stpecpy (p, end, tmp);
  }
  mpmqu = msgparseMusicQueueData (tbuff);
  ck_assert_ptr_nonnull (mpmqu);
  ck_assert_int_eq (mpmqu->mqidx, 6);
  ck_assert_int_eq (mpmqu->tottime, 2761800);
  ck_assert_int_eq (mpmqu->currdbidx, 77);
  list = mpmqu->dispList;
  for (int i = 0; i < titemsz; ++i) {
    mpmqitem = nlistGetData (list, i);
    ck_assert_ptr_nonnull (mpmqitem);
    ck_assert_int_eq (mpmqitem->dispidx, mptestdata [i].didx);
    ck_assert_int_eq (mpmqitem->uniqueidx, mptestdata [i].uidx);
    ck_assert_int_eq (mpmqitem->dbidx, mptestdata [i].dbidx);
    ck_assert_int_eq (mpmqitem->pauseind, mptestdata [i].pind);
  }
  msgparseMusicQueueDataFree (mpmqu);
}
END_TEST

START_TEST(msgparse_songsel_data)
{
  char            tbuff [BDJ4_PATH_MAX];
  mp_songselect_t *mpss = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- msgparse_songsel_data");
  mdebugSubTag ("msgparse_songsel_data");

  snprintf (tbuff, sizeof (tbuff), "%d%c%d", 2, MSG_ARGS_RS, 12);
  mpss = msgparseSongSelect (tbuff);
  ck_assert_ptr_nonnull (mpss);
  ck_assert_int_eq (mpss->mqidx, 2);
  ck_assert_int_eq (mpss->loc, 12);
  msgparseSongSelectFree (mpss);

  snprintf (tbuff, sizeof (tbuff), "%d%c%d", 4, MSG_ARGS_RS, 15);
  mpss = msgparseSongSelect (tbuff);
  ck_assert_ptr_nonnull (mpss);
  ck_assert_int_eq (mpss->mqidx, 4);
  ck_assert_int_eq (mpss->loc, 15);
  msgparseSongSelectFree (mpss);
}
END_TEST

Suite *
msgparse_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("msgparse");
  tc = tcase_create ("msgparse");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_test (tc, msgparse_mq_data);
  tcase_add_test (tc, msgparse_songsel_data);
  suite_add_tcase (s, tc);

  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
