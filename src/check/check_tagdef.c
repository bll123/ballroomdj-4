/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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

#include "check_bdj.h"
#include "log.h"
#include "tagdef.h"

START_TEST(tagdef_init)
{
  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tagdef_init");

  ck_assert_ptr_null (tagdefs [TAG_ALBUM].displayname);
  tagdefInit ();
  ck_assert_ptr_nonnull (tagdefs [TAG_ALBUM].displayname);
  tagdefInit ();
  ck_assert_ptr_nonnull (tagdefs [TAG_ALBUM].displayname);
  tagdefCleanup ();
  tagdefCleanup ();
}
END_TEST

START_TEST(tagdef_lookup)
{
  tagdefkey_t   val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tagdef_lookup");

  tagdefInit ();
  val = tagdefLookup ("ALBUM");
  ck_assert_int_eq (val, TAG_ALBUM);
  tagdefCleanup ();
}
END_TEST

Suite *
tagdef_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("tagdef");
  tc = tcase_create ("tagdef");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_test (tc, tagdef_init);
  tcase_add_test (tc, tagdef_lookup);
  suite_add_tcase (s, tc);
  return s;
}
