/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#if ! defined (BDJ4_MEM_DEBUG)
# define BDJ4_MEM_DEBUG
#endif

#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"
#include "callback.h"

typedef struct {
  int     a;
  int     b;
} ii_t;

typedef struct {
  long    a;
  int     b;
} li_t;

static bool
cbn (void *udata)
{
  if (udata != NULL) {
    int   *ival = udata;

    *ival = 1;
    return true;
  }
  return false;
}

static bool
cbd (void *udata, double val)
{
  if (udata != NULL) {
    double *dval = udata;

    *dval = val;
    return true;
  }
  return false;
}

static bool
cbii (void *udata, int a, int b)
{
  if (udata != NULL) {
    ii_t *iival = udata;

    iival->a = a;
    iival->b = b;
    return true;
  }
  return false;
}

static bool
cbl (void *udata, long val)
{
  if (udata != NULL) {
    long  *lval = udata;

    *lval = val;
    return true;
  }
  return false;
}

static int
cbli (void *udata, long a, int b)
{
  if (udata != NULL) {
    li_t *lival = udata;

    lival->a = a;
    lival->b = b;
    return true;
  }
  return false;
}

static long
cbs (void *udata, const char *txt)
{
  if (udata != NULL) {
    const char  **sval = udata;

    *sval = txt;
    return 1;
  }
  return 0;
}

START_TEST(callback_alloc)
{
  callback_t    *cb;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- callback_alloc");
  mdebugSubTag ("callback_alloc");
  cb = callbackInit (NULL, NULL, NULL);
  ck_assert_ptr_nonnull (cb);
  callbackFree (cb);

  cb = callbackInit (cbn, NULL, NULL);
  ck_assert_ptr_nonnull (cb);
  callbackFree (cb);

  cb = callbackInitDouble (cbd, NULL);
  ck_assert_ptr_nonnull (cb);
  callbackFree (cb);

  cb = callbackInitIntInt (cbii, NULL);
  ck_assert_ptr_nonnull (cb);
  callbackFree (cb);

  cb = callbackInitLong (cbl, NULL);
  ck_assert_ptr_nonnull (cb);
  callbackFree (cb);

  cb = callbackInitLongInt (cbli, NULL);
  ck_assert_ptr_nonnull (cb);
  callbackFree (cb);

  cb = callbackInitStr (cbs, NULL);
  ck_assert_ptr_nonnull (cb);
  callbackFree (cb);
}
END_TEST

START_TEST(callback_norm)
{
  callback_t    *cb;
  int           a = -1;
  bool          rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- callback_norm");
  mdebugSubTag ("callback_norm");

  cb = callbackInit (cbn, NULL, NULL);
  rc = callbackHandler (cb);
  ck_assert_int_eq (a, -1);
  ck_assert_int_eq (rc, 0);
  callbackFree (cb);

  cb = callbackInit (cbn, &a, NULL);
  rc = callbackHandler (cb);
  ck_assert_int_eq (a, 1);
  ck_assert_int_eq (rc, 1);
  callbackFree (cb);
}
END_TEST

START_TEST(callback_double)
{
  callback_t    *cb;
  double        a = -1.0;
  bool          rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- callback_double");
  mdebugSubTag ("callback_double");

  cb = callbackInitDouble (cbd, NULL);
  rc = callbackHandlerDouble (cb, 4.0);
  ck_assert_float_eq (a, -1.0);
  ck_assert_int_eq (rc, 0);
  callbackFree (cb);

  cb = callbackInitDouble (cbd, &a);
  rc = callbackHandlerDouble (cb, 5.0);
  ck_assert_double_eq (a, 5.0);
  ck_assert_int_eq (rc, 1);
  callbackFree (cb);
}
END_TEST

START_TEST(callback_intint)
{
  callback_t    *cb;
  ii_t          a;
  bool          rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- callback_intint");
  mdebugSubTag ("callback_intint");

  a.a = -1;
  a.b = -2;

  cb = callbackInitIntInt (cbii, NULL);
  rc = callbackHandlerIntInt (cb, 4, 5);
  ck_assert_int_eq (a.a, -1.0);
  ck_assert_int_eq (a.b, -2.0);
  ck_assert_int_eq (rc, 0);
  callbackFree (cb);

  cb = callbackInitIntInt (cbii, &a);
  rc = callbackHandlerIntInt (cb, 4, 5);
  ck_assert_int_eq (a.a, 4);
  ck_assert_int_eq (a.b, 5);
  ck_assert_int_eq (rc, 1);
  callbackFree (cb);
}
END_TEST

START_TEST(callback_long)
{
  callback_t    *cb;
  long          a = -1;
  bool          rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- callback_long");
  mdebugSubTag ("callback_long");

  cb = callbackInitLong (cbl, NULL);
  rc = callbackHandlerLong (cb, 4);
  ck_assert_int_eq (a, -1);
  ck_assert_int_eq (rc, 0);
  callbackFree (cb);

  cb = callbackInitLong (cbl, &a);
  rc = callbackHandlerLong (cb, 4);
  ck_assert_int_eq (a, 4);
  ck_assert_int_eq (rc, 1);
  callbackFree (cb);
}
END_TEST

START_TEST(callback_longint)
{
  callback_t    *cb;
  li_t          a;
  bool          rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- callback_longint");
  mdebugSubTag ("callback_longint");

  a.a = -1;
  a.b = -2;

  cb = callbackInitLongInt (cbli, NULL);
  rc = callbackHandlerLongInt (cb, 4, 5);
  ck_assert_int_eq (a.a, -1.0);
  ck_assert_int_eq (a.b, -2.0);
  ck_assert_int_eq (rc, 0);
  callbackFree (cb);

  cb = callbackInitLongInt (cbli, &a);
  rc = callbackHandlerLongInt (cb, 4, 5);
  ck_assert_int_eq (a.a, 4);
  ck_assert_int_eq (a.b, 5);
  ck_assert_int_eq (rc, 1);
  callbackFree (cb);
}
END_TEST

START_TEST(callback_string)
{
  callback_t    *cb;
  const char    *a;
  long          rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- callback_long");
  mdebugSubTag ("callback_long");
  a = "1234";

  cb = callbackInitStr (cbs, NULL);
  rc = callbackHandlerStr (cb, "abcdef");
  ck_assert_str_eq (a, "1234");
  ck_assert_int_eq (rc, 0);
  callbackFree (cb);

  cb = callbackInitStr (cbs, &a);
  rc = callbackHandlerStr (cb, "abcdef");
  ck_assert_str_eq (a, "abcdef");
  ck_assert_int_eq (rc, 1);
  callbackFree (cb);
}
END_TEST


Suite *
callback_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("callback");
  tc = tcase_create ("callback");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, callback_alloc);
  tcase_add_test (tc, callback_norm);
  tcase_add_test (tc, callback_double);
  tcase_add_test (tc, callback_intint);
  tcase_add_test (tc, callback_long);
  tcase_add_test (tc, callback_longint);
  tcase_add_test (tc, callback_string);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
