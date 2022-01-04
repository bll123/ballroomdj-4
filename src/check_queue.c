#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>
#if _hdr_unistd
# include <unistd.h>
#endif

#include "queue.h"
#include "check_bdj.h"
#include "portability.h"
#include "log.h"

START_TEST(queue_alloc_free)
{
  long        count;

  logMsg (LOG_DBG, LOG_LVL_1, "=== queue_alloc_free");
  queue_t *q = queueAlloc ();
  ck_assert_ptr_nonnull (q);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  queueFree (q);
}
END_TEST

START_TEST(queue_push_one)
{
  long      count;

  logMsg (LOG_DBG, LOG_LVL_1, "=== queue_push_one");
  queue_t *q = queueAlloc ();
  queuePush (q, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  queueFree (q);
}
END_TEST

START_TEST(queue_push_two)
{
  long      count;

  logMsg (LOG_DBG, LOG_LVL_1, "=== queue_push_two");
  queue_t *q = queueAlloc ();
  queuePush (q, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  queuePush (q, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);
  queueFree (q);
}
END_TEST

START_TEST(queue_push_many)
{
  long      count;

  logMsg (LOG_DBG, LOG_LVL_1, "=== queue_push_many");
  queue_t *q = queueAlloc ();
  queuePush (q, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  queuePush (q, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);
  queuePush (q, "cccc");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 3);
  queuePush (q, "dddd");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 4);
  queuePush (q, "eeee");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 5);
  queueFree (q);
}
END_TEST

START_TEST(queue_push_pop_one)
{
  long      count;
  char      *data;

  logMsg (LOG_DBG, LOG_LVL_1, "=== queue_push_pop_one");
  queue_t *q = queueAlloc ();
  queuePush (q, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  ck_assert_str_eq (data, "aaaa");
  data = queuePop (q);
  ck_assert_ptr_null (data);
  queueFree (q);
}
END_TEST

START_TEST(queue_push_pop_two)
{
  long      count;
  char      *data;

  logMsg (LOG_DBG, LOG_LVL_1, "=== queue_push_pop_two");
  queue_t *q = queueAlloc ();
  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);
  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  ck_assert_str_eq (data, "aaaa");
  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  ck_assert_str_eq (data, "bbbb");
  data = queuePop (q);
  ck_assert_ptr_null (data);
  queueFree (q);
}
END_TEST

START_TEST(queue_multi_one)
{
  long      count;
  char      *data;

  logMsg (LOG_DBG, LOG_LVL_1, "=== queue_multi_one");
  queue_t *q = queueAlloc ();

  queuePush (q, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  ck_assert_str_eq (data, "aaaa");

  data = queuePop (q);
  ck_assert_ptr_null (data);

  queuePush (q, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  ck_assert_str_eq (data, "bbbb");

  data = queuePop (q);
  ck_assert_ptr_null (data);

  queueFree (q);
}
END_TEST

START_TEST(queue_multi_two)
{
  long      count;
  char      *data;

  logMsg (LOG_DBG, LOG_LVL_1, "=== queue_multi_two");
  queue_t *q = queueAlloc ();

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  ck_assert_str_eq (data, "aaaa");
  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  ck_assert_str_eq (data, "bbbb");

  data = queuePop (q);
  ck_assert_ptr_null (data);

  queuePush (q, "cccc");
  queuePush (q, "dddd");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  ck_assert_str_eq (data, "cccc");

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  ck_assert_str_eq (data, "dddd");

  queuePush (q, "eeee");
  queuePush (q, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  ck_assert_str_eq (data, "eeee");

  queuePush (q, "eeee");

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  ck_assert_str_eq (data, "ffff");

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  ck_assert_str_eq (data, "eeee");

  data = queuePop (q);
  ck_assert_ptr_null (data);

  queueFree (q);
}
END_TEST

START_TEST(queue_multi_many)
{
  long      count;
  char      *data;

  logMsg (LOG_DBG, LOG_LVL_1, "=== queue_multi_two");
  queue_t *q = queueAlloc ();

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "gggg");
  queuePush (q, "hhhh");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 4);

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 3);
  ck_assert_str_eq (data, "aaaa");
  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);
  ck_assert_str_eq (data, "bbbb");

  queuePush (q, "cccc");
  queuePush (q, "dddd");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 4);

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 3);
  ck_assert_str_eq (data, "gggg");

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);
  ck_assert_str_eq (data, "hhhh");

  queuePush (q, "eeee");
  queuePush (q, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 4);

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 3);
  ck_assert_str_eq (data, "cccc");

  queuePush (q, "eeee");

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 3);
  ck_assert_str_eq (data, "dddd");

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);
  ck_assert_str_eq (data, "eeee");

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  ck_assert_str_eq (data, "ffff");

  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  ck_assert_str_eq (data, "eeee");

  data = queuePop (q);
  ck_assert_ptr_null (data);

  queueFree (q);
}
END_TEST

Suite *
queue_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Queue Suite");
  tc = tcase_create ("Queue Op");
  tcase_add_test (tc, queue_alloc_free);
  tcase_add_test (tc, queue_push_one);
  tcase_add_test (tc, queue_push_two);
  tcase_add_test (tc, queue_push_many);
  tcase_add_test (tc, queue_push_pop_one);
  tcase_add_test (tc, queue_push_pop_two);
  tcase_add_test (tc, queue_multi_one);
  tcase_add_test (tc, queue_multi_two);
  tcase_add_test (tc, queue_multi_many);
  suite_add_tcase (s, tc);
  return s;
}

