/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"
#include "ossignal.h"

static int gsig = 0;

static void sigh (int sig);


START_TEST(ossignal_chk)
{
  int   count = 0;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ossignal_chk");
  mdebugSubTag ("ossignal_chk");

  osSetStandardSignals (sigh);

  ck_assert_int_eq (gsig, 0);
#if _define_SIGHUP && _lib_kill
  kill (getpid (), SIGHUP);
  ++count;
  ck_assert_int_eq (gsig, count);
#endif

  osIgnoreSignal (SIGTERM);
#if _lib_kill
  kill (getpid (), SIGTERM);
#endif
  ck_assert_int_eq (gsig, count);

  osCatchSignal (sigh, SIGTERM);
#if _lib_kill
  kill (getpid (), SIGTERM);
  ++count;
#endif
  ck_assert_int_eq (gsig, count);

  osDefaultSignal (SIGTERM);
  osCatchSignal (sigh, SIGTERM);
#if _lib_kill
  kill (getpid (), SIGTERM);
  ++count;
#endif
  ck_assert_int_eq (gsig, count);
}
END_TEST


Suite *
ossignal_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("ossignal");
  tc = tcase_create ("ossignal");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, ossignal_chk);
  suite_add_tcase (s, tc);
  return s;
}

static void
sigh (int sig)
{
  gsig++;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
