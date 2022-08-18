#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjmsg.h"
#include "check_bdj.h"

START_TEST(bdjmsg_encode)
{
  char    buff [2048];

  msgEncode (ROUTE_MAIN, ROUTE_STARTERUI, MSG_EXIT_REQUEST,
      "def", buff, sizeof (buff));
  ck_assert_str_eq (buff, "0004~0012~0001~def");
}
END_TEST

START_TEST(bdjmsg_decode)
{
  char    buff [2048];
  bdjmsgroute_t rf;
  bdjmsgroute_t rt;
  bdjmsgmsg_t   msg;
  char    args [200];

  msgEncode (ROUTE_MAIN, ROUTE_STARTERUI, MSG_EXIT_REQUEST,
      "def", buff, sizeof (buff));
  msgDecode (buff, &rf, &rt, &msg, args, sizeof (args));
  ck_assert_int_eq (rf, ROUTE_MAIN);
  ck_assert_int_eq (rt, ROUTE_STARTERUI);
  ck_assert_int_eq (msg, MSG_EXIT_REQUEST);
  ck_assert_str_eq (args, "def");
}
END_TEST

Suite *
bdjmsg_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("bdjmsg");
  tc = tcase_create ("bdjmsg");
  tcase_add_test (tc, bdjmsg_encode);
  tcase_add_test (tc, bdjmsg_decode);
  suite_add_tcase (s, tc);
  return s;
}

