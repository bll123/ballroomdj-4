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

#include "queue.h"
#include "check_bdj.h"
#include "log.h"

START_TEST(queue_alloc_free)
{
  qidx_t        count;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_alloc_free");
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
  q = queueAlloc ("push-one", NULL);
  queuePush (q, "aaaa");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 1);
  queueFree (q);
}
END_TEST

START_TEST(queue_push_two)
{
  qidx_t        count;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_push_two");
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

START_TEST(queue_get_current)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_get_current");
  q = queueAlloc ("get-curr", NULL);
  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 2);

  data = queueGetCurrent (q);
  count = queueGetCount (q);
  ck_assert_ptr_nonnull (data);
  ck_assert_int_eq (count, 2);
  ck_assert_str_eq (data, "aaaa");

  data = queuePop (q);
  ck_assert_str_eq (data, "aaaa");

  data = queueGetCurrent (q);
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

START_TEST(queue_iterate)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;
  qidx_t    qiteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_iterate");
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

START_TEST(queue_remove_node)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;
  qidx_t    qiteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_remove_node");
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

START_TEST(queue_getbyidx)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;
  ssize_t   i;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_getbyidx");
  q = queueAlloc ("get-by-idx", NULL);

  queuePush (q, "aaaa");
  queuePush (q, "bbbb");
  queuePush (q, "cccc");
  queuePush (q, "dddd");
  queuePush (q, "eeee");
  queuePush (q, "ffff");
  count = queueGetCount (q);
  ck_assert_int_eq (count, 6);

  i = 0;
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  /* should be cached */
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "aaaa");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "bbbb");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "cccc");
  data = queueGetByIdx (q, i);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  /* should be cached */
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "dddd");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "eeee");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_nonnull (data);
  ck_assert_str_eq (data, "ffff");
  data = queueGetByIdx (q, i++);
  ck_assert_ptr_null (data);

  queueFree (q);
}
END_TEST

START_TEST(queue_clear)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;
  ssize_t   i;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_clear");
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


START_TEST(queue_move)
{
  qidx_t    count;
  char      *data;
  queue_t       *q;
  ssize_t   i;
  qidx_t    qiteridx;


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_move");
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
  queue_t       *q;
  qidx_t    qiteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- queue_insert_node");
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
  tcase_add_test (tc, queue_push_two);
  tcase_add_test (tc, queue_push_many);
  tcase_add_test (tc, queue_push_pop_one);
  tcase_add_test (tc, queue_push_pop_two);
  tcase_add_test (tc, queue_push_head);
  tcase_add_test (tc, queue_get_current);
  tcase_add_test (tc, queue_multi_one);
  tcase_add_test (tc, queue_multi_two);
  tcase_add_test (tc, queue_multi_many);
  tcase_add_test (tc, queue_iterate);
  tcase_add_test (tc, queue_remove_by_idx);
  tcase_add_test (tc, queue_remove_node);
  tcase_add_test (tc, queue_getbyidx);
  tcase_add_test (tc, queue_clear);
  tcase_add_test (tc, queue_move);
  tcase_add_test (tc, queue_insert_node);
  suite_add_tcase (s, tc);

  return s;
}

