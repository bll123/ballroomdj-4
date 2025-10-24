/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

#include "bdjopt.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"
#include "musicq.h"
#include "tmutil.h"

START_TEST(bdjopt_conv_bpm)
{
  datafileconv_t conv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjopt_conv_bpm");
  mdebugSubTag ("bdjopt_conv_bpm");

  for (int i = BPM_BPM; i < BPM_MAX; ++i) {
    conv.invt = VALUE_NUM;
    conv.num = i;
    bdjoptConvBPM (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_STR);
    ck_assert_ptr_nonnull (conv.str);
    conv.invt = VALUE_STR;
    bdjoptConvBPM (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_NUM);
    ck_assert_int_eq (conv.num, i);
  }
}
END_TEST

START_TEST(bdjopt_conv_clock)
{
  datafileconv_t conv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjopt_conv_bpm");
  mdebugSubTag ("bdjopt_conv_bpm");

  for (int i = TM_CLOCK_ISO; i < TM_CLOCK_MAX; ++i) {
    conv.invt = VALUE_NUM;
    conv.num = i;
    bdjoptConvClock (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_STR);
    ck_assert_ptr_nonnull (conv.str);
    conv.invt = VALUE_STR;
    bdjoptConvClock (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_NUM);
    ck_assert_int_eq (conv.num, i);
  }
}
END_TEST

START_TEST(bdjopt_conv_fadetype)
{
  datafileconv_t conv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjopt_conv_fadetype");
  mdebugSubTag ("bdjopt_conv_fadetype");

  for (int i = FADETYPE_EXPONENTIAL_SINE; i < FADETYPE_MAX; ++i) {
    conv.invt = VALUE_NUM;
    conv.num = i;
    bdjoptConvFadeType (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_STR);
    ck_assert_ptr_nonnull (conv.str);
    conv.invt = VALUE_STR;
    bdjoptConvFadeType (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_NUM);
    ck_assert_int_eq (conv.num, i);
  }
}
END_TEST

START_TEST(bdjopt_conv_writetags)
{
  datafileconv_t conv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjopt_conv_writetags");
  mdebugSubTag ("bdjopt_conv_writetags");

  for (int i = WRITE_TAGS_NONE; i < WRITE_TAGS_MAX; ++i) {
    conv.invt = VALUE_NUM;
    conv.num = i;
    bdjoptConvWriteTags (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_STR);
    ck_assert_ptr_nonnull (conv.str);
    conv.invt = VALUE_STR;
    bdjoptConvWriteTags (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_NUM);
    ck_assert_int_eq (conv.num, i);
  }
}
END_TEST

START_TEST(bdjopt_conv_mqshow)
{
  datafileconv_t conv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjopt_conv_mqshow");
  mdebugSubTag ("bdjopt_conv_mqshow");

  for (int i = BDJWIN_SHOW_OFF; i < BDJWIN_SHOW_MAX; ++i) {
    conv.invt = VALUE_NUM;
    conv.num = i;
    bdjoptConvBDJWinShow (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_STR);
    ck_assert_ptr_nonnull (conv.str);
    conv.invt = VALUE_STR;
    bdjoptConvBDJWinShow (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_NUM);
    ck_assert_int_eq (conv.num, i);
  }
}
END_TEST

START_TEST(bdjopt_conv_dancesel_method)
{
  datafileconv_t conv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjopt_conv_dancesel_method");
  mdebugSubTag ("bdjopt_conv_dancesel_method");

  for (int i = DANCESEL_METHOD_WINDOWED; i < DANCESEL_METHOD_MAX; ++i) {
    conv.invt = VALUE_NUM;
    conv.num = i;
    bdjoptConvDanceselMethod (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_STR);
    ck_assert_ptr_nonnull (conv.str);
    conv.invt = VALUE_STR;
    bdjoptConvDanceselMethod (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_NUM);
    ck_assert_int_eq (conv.num, i);
  }
}
END_TEST

START_TEST(bdjopt_conv_mobmq)
{
  datafileconv_t conv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjopt_conv_mobmq");
  mdebugSubTag ("bdjopt_conv_mobmq");

  for (int i = MOBMQ_TYPE_OFF; i < MOBMQ_TYPE_MAX; ++i) {
    conv.invt = VALUE_NUM;
    conv.num = i;
    bdjoptConvMobMQType (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_STR);
    ck_assert_ptr_nonnull (conv.str);
    conv.invt = VALUE_STR;
    bdjoptConvMobMQType (&conv);
    ck_assert_int_eq (conv.outvt, VALUE_NUM);
    ck_assert_int_eq (conv.num, i);
  }
}
END_TEST

START_TEST(bdjopt_get)
{
  const char  *tstr;
  int         val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjopt_get");
  mdebugSubTag ("bdjopt_get");

  bdjoptInit ();
  tstr = bdjoptGetStr (OPT_G_ACOUSTID_KEY);
  ck_assert_ptr_nonnull (tstr);
  tstr = bdjoptGetStr (OPT_M_DIR_MUSIC);
  ck_assert_ptr_nonnull (tstr);
  tstr = bdjoptGetStr (OPT_M_UI_FONT);
  ck_assert_ptr_nonnull (tstr);
  val = bdjoptGetNum (OPT_P_FADETYPE);
  ck_assert_int_ge (val, 0);
  val = bdjoptGetNumPerQueue (OPT_Q_START_WAIT_TIME, MUSICQ_PB_A);
  ck_assert_int_ge (val, 0);
  val = bdjoptGetNumPerQueue (OPT_Q_START_WAIT_TIME, MUSICQ_PB_B);
  ck_assert_int_ge (val, 0);
  val = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_A);
  ck_assert_int_ge (val, 0);
  val = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_B);
  ck_assert_int_ge (val, 0);
  tstr = bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, MUSICQ_PB_A);
  ck_assert_ptr_nonnull (tstr);
  bdjoptCleanup ();
}
END_TEST

START_TEST(bdjopt_set)
{
  char        *ostr;
  char        *oqn;
  const char  *tstr;
  int         oval, ovalb, oact;
  int         val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjopt_set");
  mdebugSubTag ("bdjopt_set");

  bdjoptInit ();

  oval = bdjoptGetNum (OPT_P_FADETYPE);
  ck_assert_int_ge (oval, 0);
  bdjoptSetNum (OPT_P_FADETYPE, 1);
  val = bdjoptGetNum (OPT_P_FADETYPE);
  ck_assert_int_eq (val, 1);
  bdjoptSetNum (OPT_P_FADETYPE, oval);
  val = bdjoptGetNum (OPT_P_FADETYPE);
  ck_assert_int_eq (val, oval);

  ostr = mdstrdup (bdjoptGetStr (OPT_P_COMPLETE_MSG));
  bdjoptSetStr (OPT_P_COMPLETE_MSG, "hello");
  tstr = bdjoptGetStr (OPT_P_COMPLETE_MSG);
  ck_assert_str_eq (tstr, "hello");
  bdjoptSetStr (OPT_P_COMPLETE_MSG, ostr);
  tstr = bdjoptGetStr (OPT_P_COMPLETE_MSG);
  ck_assert_str_eq (tstr, ostr);

  oval = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_A);
  ck_assert_int_ge (oval, 0);
  ovalb = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_B);
  ck_assert_int_ge (ovalb, 0);
  oact = bdjoptGetNumPerQueue (OPT_Q_ACTIVE, MUSICQ_PB_B);
  ck_assert_int_ge (oact, 0);
  oqn = mdstrdup (bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, MUSICQ_PB_B));
  ck_assert_ptr_nonnull (ostr);

  bdjoptSetNumPerQueue (OPT_Q_GAP, 2000, MUSICQ_PB_A);
  bdjoptSetNumPerQueue (OPT_Q_GAP, 3000, MUSICQ_PB_B);
  bdjoptSetNumPerQueue (OPT_Q_START_WAIT_TIME, 1000, MUSICQ_PB_A);
  bdjoptSetNumPerQueue (OPT_Q_START_WAIT_TIME, 2000, MUSICQ_PB_B);
  bdjoptSetStrPerQueue (OPT_Q_QUEUE_NAME, "testb", MUSICQ_PB_B);

  val = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_A);
  ck_assert_int_eq (val, 2000);
  /* if the queue is not active, the value for pb-a is returned */
  bdjoptSetNumPerQueue (OPT_Q_ACTIVE, false, MUSICQ_PB_B);
  val = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_B);
  ck_assert_int_eq (val, 2000);
  val = bdjoptGetNumPerQueue (OPT_Q_START_WAIT_TIME, MUSICQ_PB_A);
  ck_assert_int_eq (val, 1000);
  /* queue-b is not active, the value will be from queue-a */
  val = bdjoptGetNumPerQueue (OPT_Q_START_WAIT_TIME, MUSICQ_PB_B);
  ck_assert_int_eq (val, 1000);
  tstr = bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, MUSICQ_PB_B);
  ck_assert_str_eq (tstr, "testb");

  /* if the queue is active, the set value is returned */
  bdjoptSetNumPerQueue (OPT_Q_ACTIVE, true, MUSICQ_PB_B);
  val = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_B);
  ck_assert_int_eq (val, 3000);
  val = bdjoptGetNumPerQueue (OPT_Q_START_WAIT_TIME, MUSICQ_PB_A);
  ck_assert_int_eq (val, 1000);
  /* queue-b is now active, the value will be from queue-b */
  val = bdjoptGetNumPerQueue (OPT_Q_START_WAIT_TIME, MUSICQ_PB_B);
  ck_assert_int_eq (val, 2000);
  tstr = bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, MUSICQ_PB_B);
  ck_assert_str_eq (tstr, "testb");

  bdjoptSetNumPerQueue (OPT_Q_GAP, oval, MUSICQ_PB_A);
  bdjoptSetNumPerQueue (OPT_Q_GAP, ovalb, MUSICQ_PB_B);
  bdjoptSetNumPerQueue (OPT_Q_ACTIVE, oact, MUSICQ_PB_B);
  bdjoptSetStrPerQueue (OPT_Q_QUEUE_NAME, oqn, MUSICQ_PB_B);
  val = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_A);
  ck_assert_int_eq (val, oval);
  val = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_B);
  ck_assert_int_eq (val, ovalb);
  val = bdjoptGetNumPerQueue (OPT_Q_ACTIVE, MUSICQ_PB_B);
  ck_assert_int_eq (val, oact);
  tstr = bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, MUSICQ_PB_B);
  ck_assert_str_eq (tstr, oqn);

  bdjoptCleanup ();

  mdfree (ostr);
  mdfree (oqn);
}
END_TEST

START_TEST(bdjopt_save)
{
  char        *ostr;
  char        *oqn;
  const char  *tstr;
  int         oval, ogap, oact;
  int         val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjopt_save");
  mdebugSubTag ("bdjopt_save");

  bdjoptInit ();

  oval = bdjoptGetNum (OPT_P_FADETYPE);
  ck_assert_int_ge (oval, 0);
  bdjoptSetNum (OPT_P_FADETYPE, 1);
  val = bdjoptGetNum (OPT_P_FADETYPE);
  ck_assert_int_eq (val, 1);

  ostr = mdstrdup (bdjoptGetStr (OPT_P_COMPLETE_MSG));
  bdjoptSetStr (OPT_P_COMPLETE_MSG, "hello");
  tstr = bdjoptGetStr (OPT_P_COMPLETE_MSG);
  ck_assert_str_eq (tstr, "hello");

  oact = bdjoptGetNumPerQueue (OPT_Q_ACTIVE, MUSICQ_PB_B);
  ck_assert_int_ge (oact, 0);
  bdjoptSetNumPerQueue (OPT_Q_ACTIVE, true, MUSICQ_PB_B);
  ogap = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_B);
  ck_assert_int_ge (ogap, 0);
  bdjoptSetNumPerQueue (OPT_Q_GAP, 3000, MUSICQ_PB_B);
  oqn = mdstrdup (bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, MUSICQ_PB_B));
  bdjoptSetStrPerQueue (OPT_Q_QUEUE_NAME, "testc", MUSICQ_PB_B);

  val = bdjoptGetNumPerQueue (OPT_Q_ACTIVE, MUSICQ_PB_B);
  ck_assert_int_eq (val, true);
  val = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_B);
  ck_assert_int_eq (val, 3000);
  tstr = bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, MUSICQ_PB_B);
  ck_assert_str_eq (tstr, "testc");

  bdjoptSave ();
  bdjoptCleanup ();
  bdjoptInit ();

  val = bdjoptGetNum (OPT_P_FADETYPE);
  ck_assert_int_eq (val, 1);
  bdjoptSetNum (OPT_P_FADETYPE, oval);

  tstr = bdjoptGetStr (OPT_P_COMPLETE_MSG);
  ck_assert_str_eq (tstr, "hello");
  bdjoptSetStr (OPT_P_COMPLETE_MSG, ostr);

  val = bdjoptGetNumPerQueue (OPT_Q_GAP, MUSICQ_PB_B);
  ck_assert_int_eq (val, 3000);
  tstr = bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, MUSICQ_PB_B);
  ck_assert_str_eq (tstr, "testc");
  bdjoptSetNumPerQueue (OPT_Q_GAP, ogap, MUSICQ_PB_B);
  bdjoptSetNumPerQueue (OPT_Q_ACTIVE, oact, MUSICQ_PB_B);
  bdjoptSetStrPerQueue (OPT_Q_QUEUE_NAME, oqn, MUSICQ_PB_B);

  mdfree (ostr);
  mdfree (oqn);

  bdjoptSave ();
  bdjoptCleanup ();
}
END_TEST

Suite *
bdjopt_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("bdjopt");
  tc = tcase_create ("bdjopt");
  tcase_set_tags (tc, "libbasic");
  tcase_add_test (tc, bdjopt_conv_bpm);
  tcase_add_test (tc, bdjopt_conv_clock);
  tcase_add_test (tc, bdjopt_conv_fadetype);
  tcase_add_test (tc, bdjopt_conv_writetags);
  tcase_add_test (tc, bdjopt_conv_mqshow);
  tcase_add_test (tc, bdjopt_conv_dancesel_method);
  tcase_add_test (tc, bdjopt_conv_mobmq);
  tcase_add_test (tc, bdjopt_get);
  tcase_add_test (tc, bdjopt_set);
  tcase_add_test (tc, bdjopt_save);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
