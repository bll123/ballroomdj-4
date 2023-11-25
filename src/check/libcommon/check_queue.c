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

#include "queue.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"

START_TEST(queue_alloc_free)
{
  qidx_t        count;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_alloc_free");
  mdebugSubTag ("queue_alloc_free");
  q = queueAlloc ("alloc-free", NULL);
  ck_assert_ptr_nonnull (q);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  queueFree (q);
}
END_TEST

START_TEST(queue_push_one)
{
  qidx_t        count;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_push_one");
  mdebugSubTag ("queue_push_one");
  q = queueAlloc ("push-one", NULL);
  queuePush (q, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  queueFree (q);
}
END_TEST

START_TEST(queue_access_after_free)
{
  qidx_t        count;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_push_one");
  mdebugSubTag ("queue_push_one");
  q = queueAlloc ("push-one", NULL);
  queuePush (q, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  queueFree (q);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
}
END_TEST

START_TEST(queue_bad_access)
{
  qidx_t        count;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_push_one");
  mdebugSubTag ("queue_push_one");
  q = queueAlloc ("push-one", NULL);
  queuePush (q, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  queueFree (q);
  q = (queue_t *) &count;
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
}
END_TEST

START_TEST(queue_push_two)
{
  qidx_t        count;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_push_two");
  mdebugSubTag ("queue_push_two");
  q = queueAlloc ("push-two", NULL);
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
  qidx_t        count;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_push_many");
  mdebugSubTag ("queue_push_many");
  q = queueAlloc ("push-many", NULL);
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
  qidx_t    count;
  char      *data;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_push_pop_one");
  mdebugSubTag ("queue_push_pop_one");
  q = queueAlloc ("push-pop-1", NULL);
  queuePush (q, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
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
  qidx_t    count;
  char      *data;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_push_pop_two");
  mdebugSubTag ("queue_push_pop_two");
  q = queueAlloc ("push-pop-2", NULL);
  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);
  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
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

START_TEST(queue_push_head)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_push_head");
  mdebugSubTag ("queue_push_head");
  q = queueAlloc ("push-head", NULL);
  queuePush (q, "aaaa");
  queuePushHead (q, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);
  data = queuePop (q);
  ck_assert_ptr_nonnull (data);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  ck_assert_str_eq (data, "bbbb");
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

START_TEST(queue_get_first)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_get_first");
  mdebugSubTag ("queue_get_first");
  q = queueAlloc ("get-first", NULL);
  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);

  data = queueGetFirst (q);
  count = queueGetCount (q);
  ck_assert_ptr_nonnull (data);
  ck_assert_int_eq (count, 2);
  ck_assert_str_eq (data, "aaaa");

  data = queuePop (q);
  ck_assert_str_eq (data, "aaaa");

  queuePushHead (q, "cccc");
  data = queueGetFirst (q);
  count = queueGetCount (q);
  ck_assert_ptr_nonnull (data);
  ck_assert_int_eq (count, 2);
  ck_assert_str_eq (data, "cccc");

  data = queuePop (q);
  ck_assert_str_eq (data, "cccc");

  data = queueGetFirst (q);
  count = queueGetCount (q);
  ck_assert_ptr_nonnull (data);
  ck_assert_int_eq (count, 1);
  ck_assert_str_eq (data, "bbbb");

  data = queuePop (q);
  ck_assert_str_eq (data, "bbbb");
  data = queuePop (q);
  ck_assert_ptr_null (data);
  queueFree (q);
}
END_TEST

START_TEST(queue_multi_one)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_multi_one");
  mdebugSubTag ("queue_multi_one");
  q = queueAlloc ("multi-1", NULL);

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
  qidx_t    count;
  char      *data;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_multi_two");
  mdebugSubTag ("queue_multi_two");
  q = queueAlloc ("multi-2", NULL);

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
  qidx_t    count;
  char      *data;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_multi_many");
  mdebugSubTag ("queue_multi_many");
  q = queueAlloc ("multi-many", NULL);

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

START_TEST(queue_getbyidx)
{
  qidx_t    count;
  char      *data;
  queue_t   *q;
  qidx_t    i;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_getbyidx");
  mdebugSubTag ("queue_getbyidx");
  q = queueAlloc ("get-by-idx", NULL);

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "cccc");
  queuePush (q, "dddd");
  queuePush (q, "eeee");
  queuePush (q, "ffff");
  queuePush (q, "gggg");
  queuePush (q, "hhhh");
  queuePush (q, "iiii");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 9);

  i = 0;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  /* search dist 0 */
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  /* should be cached, dist 0 */
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  /* should be cached */
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  /* should be cached, dist 0 */
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "gggg");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "hhhh");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "iiii");
  /* last item, search dist 0 */
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_null (data);

  queueFree (q);
}

START_TEST(queue_iterate)
{
  qidx_t    count;
  char      *data;
  queue_t   *q;
  qidx_t    qiteridx;
  qidx_t    i;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_iterate");
  mdebugSubTag ("queue_iterate");
  q = queueAlloc ("iterate", NULL);

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "cccc");
  queuePush (q, "dddd");
  queuePush (q, "eeee");
  queuePush (q, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 6);

  queueStartIterator (q, &qiteridx);

  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");

  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");

  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");

  /* should start at cache, should have a search distance of 0 */
  i = 2;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");

  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");

  /* should start at cache, should have a search distance of 0 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");

  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  queueFree (q);
}
END_TEST

START_TEST(queue_move)
{
  qidx_t    count;
  char      *data;
  queue_t   *q;
  qidx_t    i;
  qidx_t    qiteridx;


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_move");
  mdebugSubTag ("queue_move");
  q = queueAlloc ("move", NULL);

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "cccc");
  queuePush (q, "dddd");
  queuePush (q, "eeee");
  queuePush (q, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 6);

  /* invalid */
  queueMove (q, 1, -1);
  i = 0;
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");

  /* invalid */
  queueMove (q, 0, 6);
  i = 0;
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  i = 5;
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");

  queueMove (q, 1, 0);
  i = 0;
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");

  queueMove (q, 4, 5);
  i = 4;
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");

  queueMove (q, 2, 3);
  i = 2;
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");

  queueMove (q, 0, 5);

  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  queueFree (q);
}
END_TEST

START_TEST(queue_insert_node)
{
  qidx_t    count;
  char      *data;
  queue_t   *q;
  qidx_t    qiteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_insert_node");
  mdebugSubTag ("queue_insert_node");
  q = queueAlloc ("insert", NULL);

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "cccc");
  queuePush (q, "dddd");
  queuePush (q, "eeee");
  queuePush (q, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 6);

  // insert into middle
  queueInsert (q, 3, "zzzz");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 7);

  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "zzzz");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // insert at 1
  queueInsert (q, 1, "wwww");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 8);

  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "wwww");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "zzzz");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // insert at 0
  queueInsert (q, 0, "xxxx");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 9);

  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "xxxx");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "wwww");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "zzzz");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  queueFree (q);
}
END_TEST

START_TEST(queue_remove_by_idx)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;
  qidx_t    qiteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_remove_by_idx");
  mdebugSubTag ("queue_remove_by_idx");
  q = queueAlloc ("remove-by-idx", NULL);

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "cccc");
  queuePush (q, "dddd");
  queuePush (q, "eeee");
  queuePush (q, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 6);

  // remove from middle
  data = queueRemoveByIdx (q, 3);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 5);
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove head
  data = queueRemoveByIdx (q, 0);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 4);
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove tail
  data = queueRemoveByIdx (q, 3);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 3);
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove fail
  data = queueRemoveByIdx (q, 3);
  ck_assert_ptr_null (data);

  // remove middle
  data = queueRemoveByIdx (q, 1);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove head
  data = queueRemoveByIdx (q, 0);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove head
  data = queueRemoveByIdx (q, 0);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  queueFree (q);
}
END_TEST

/* has all the cache tests */
/* uses some queue operations that have not yet been tested */
START_TEST(queue_cache)
{
  qidx_t    count;
  char      *data;
  queue_t   *q;
  qidx_t    i;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_cache");
  mdebugSubTag ("queue_cache");
  q = queueAlloc ("get-by-idx", NULL);

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "cccc");
  queuePush (q, "dddd");
  queuePush (q, "eeee");
  queuePush (q, "ffff");
  queuePush (q, "gggg");
  queuePush (q, "hhhh");
  queuePush (q, "iiii");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 9);

  i = 0;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  /* search dist 0 */
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  /* should be cached, dist 0 */
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  /* should be cached */
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  /* should be cached, dist 0 */
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "gggg");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "hhhh");
  /* search dist 1 */
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "iiii");
  /* last item, search dist 0 */
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  data = queueGetByIdx (q, i++);
  ck_assert_ptr_null (data);

  /* cache checks */

  /* should start at head, should have a search distance of 0 */
  i = 0;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  /* should start at tail, should have a search distance of 0 */
  i = queueGetCount (q) - 1;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "iiii");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  /* should start at head, should have a search distance of 2 (next) */
  i = 2;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  ck_assert_int_eq (queueDebugSearchDist (q), 2);

  /* should start at tail, should have a search distance of 2 (prev) */
  i = queueGetCount(q) - 1 - 2;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "gggg");
  ck_assert_int_eq (queueDebugSearchDist (q), 2);

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");

  /* should start at cache, should have a search distance of 1 (next) */
  i = 5;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");

  /* should start at cache, should have a search distance of 1 (prev) */
  i = 3;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");

  /* should start at tail, should have a search distance of 1 */
  i = queueGetCount(q) - 1 - 1;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "hhhh");
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");

  /* should start at cache, should have a search distance of 0 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");

  /* get idx 0, should not change the cache */
  i = 0;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  /* should start at cache, should have a search distance of 0 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  /* get last idx, should not change the cache */
  i = queueGetCount (q) - 1;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "iiii");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  /* should start at cache, should have a search distance of 0 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  /* cache invalidations */

  /* invalidate by push head */

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");

  /* a push head invalidates the cache */
  queuePushHead (q, "jjjj");
  /* j a b c d e f g h i */

  /* should start at head, should have a search distance of 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  ck_assert_int_eq (queueDebugSearchDist (q), 4);

  /* invalidate by pop */

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");

  /* a pop invalidates the cache */
  (void) ! queuePop (q);
  /* a b c d e f g h i */

  /* should start at head, should have a search distance of 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  ck_assert_int_eq (queueDebugSearchDist (q), 4);

  /* move */

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");

  /* a move does not invalidate the cache */
  /* the cache contains the node, not the data */
  /* the cache will be updated to point at the to-node (if not head/tail) */
  queueMove (q, 3, 4);
  /* a b c e d f g h i */

  /* should start at cache, should have a search distance of 0 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  /* a move does not invalidate the cache */
  /* the cache contains the node, not the data */
  /* the cache will be updated to point at the to-node (if not head/tail) */
  queueMove (q, 5, 6);
  /* a b c e d g f h i */

  /* should start at cache, should have a search distance of 2 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  ck_assert_int_eq (queueDebugSearchDist (q), 2);

  /* insert */

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");

  /* an insert changes the cache to point at the inserted node */
  queueInsert (q, 3, "kkkk");
  /* a b c k e d g f h i */

  /* should start at cache, should have a search distance of 1 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  ck_assert_int_eq (queueDebugSearchDist (q), 1);

  /* invalidate by removal */

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");

  /* a removal invalidates the cache */
  queueRemoveByIdx (q, 3);
  /* a b c e d g f h i */

  /* should start at head, should have a search distance of 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  ck_assert_int_eq (queueDebugSearchDist (q), 4);

  /* get idx 4 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");

  /* an push does not modify the cache */
  /* the cache contains the node, not the data */
  queuePush (q, "llll");
  /* a b c e d g f h i l */

  /* should start at cache, should have a search distance of 0 */
  i = 4;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  ck_assert_int_eq (queueDebugSearchDist (q), 0);

  queueFree (q);
}
END_TEST

START_TEST(queue_remove_iter_node)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;
  qidx_t    qiteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_remove_iter_node");
  mdebugSubTag ("queue_remove_iter_node");
  q = queueAlloc ("remove-node", NULL);

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "cccc");
  queuePush (q, "dddd");
  queuePush (q, "eeee");
  queuePush (q, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 6);

  // remove from middle
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  data = queueIterateRemoveNode (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 5);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove from head
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  data = queueIterateRemoveNode (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 4);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove tail
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueIterateRemoveNode (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 3);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove fail
  data = queueIterateRemoveNode (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove middle
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  data = queueIterateRemoveNode (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove head
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueIterateRemoveNode (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  // remove head
  queueStartIterator (q, &qiteridx);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueIterateRemoveNode (q, &qiteridx);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);
  data = queueIterateData (q, &qiteridx);
  ck_assert_ptr_null (data);

  queueFree (q);
}
END_TEST

START_TEST(queue_clear)
{
  qidx_t    count;
  char      *data;
  queue_t   *q;
  qidx_t    i;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_clear");
  mdebugSubTag ("queue_clear");
  q = queueAlloc ("clear", NULL);

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "cccc");
  queuePush (q, "dddd");
  queuePush (q, "eeee");
  queuePush (q, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 6);

  queueClear (q, 0);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 0);

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "cccc");
  queuePush (q, "dddd");
  queuePush (q, "eeee");
  queuePush (q, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 6);

  queueClear (q, 3);
  count = queueGetCount (q);
  ck_assert_int_eq (count, 3);

  i = 0;
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_null (data);

  queueFree (q);
}
END_TEST

Suite *
queue_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("queue");
  tc = tcase_create ("queue");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, queue_alloc_free);
  tcase_add_test (tc, queue_push_one);
  tcase_add_test (tc, queue_access_after_free);
  tcase_add_test (tc, queue_bad_access);
  tcase_add_test (tc, queue_push_two);
  tcase_add_test (tc, queue_push_many);
  tcase_add_test (tc, queue_push_pop_one);
  tcase_add_test (tc, queue_push_pop_two);
  tcase_add_test (tc, queue_push_head);
  tcase_add_test (tc, queue_get_first);
  tcase_add_test (tc, queue_multi_one);
  tcase_add_test (tc, queue_multi_two);
  tcase_add_test (tc, queue_multi_many);
  tcase_add_test (tc, queue_getbyidx);
  tcase_add_test (tc, queue_iterate);
  tcase_add_test (tc, queue_move);
  tcase_add_test (tc, queue_insert_node);
  tcase_add_test (tc, queue_remove_by_idx);
  tcase_add_test (tc, queue_cache);
  tcase_add_test (tc, queue_remove_iter_node);
  tcase_add_test (tc, queue_clear);
  suite_add_tcase (s, tc);

  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
