/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
/*
 * queue.c
 *
 * A queue is a doubly-linked list.
 * It is used when insertions/removals/moves are needed.
 *
 * Fetching a particular item by index is not nearly as efficient
 * as using a list.  Therefore queues should only be used when
 * insertions/removals/moves are required, and should not be used
 * for large numbers of items.
 *
 * Optimizations are in place to cache indexes and perform the shortest
 * search.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "bdjstring.h"
#include "log.h"
#include "mdebug.h"
#include "queue.h"

enum {
  QUEUE_NO_IDX = -1,
  Q_DIR_PREV = -1,
  Q_DIR_NEXT = 1,
};

enum {
  QUEUE_IDENT = 0xbbaa006575657571,
};

typedef struct queuenode {
  void              *data;
  struct queuenode  *prev;
  struct queuenode  *next;
} queuenode_t;

typedef struct queue {
  uint64_t      queueident;
  char          *name;
  qidx_t        count;
  queuenode_t   *cacheNode;
  qidx_t        cacheIdx;
  long          cacheHits;
  queuenode_t   *iteratorNode;
  queuenode_t   *currentNode;
  queuenode_t   *head;
  queuenode_t   *tail;
  queueFree_t   freeHook;
  int           searchDist;
} queue_t;

static queuenode_t * queueGetNodeByIdx (queue_t *q, qidx_t idx);
static void * queueRemove (queue_t *q, queuenode_t *node);
static void queueFreeNodeData (queue_t *q, queuenode_t *node);

queue_t *
queueAlloc (const char *name, queueFree_t freeHook)
{
  queue_t     *q;

  logProcBegin ();
  q = mdmalloc (sizeof (queue_t));
  q->queueident = QUEUE_IDENT;
  q->name = mdstrdup (name);
  q->count = 0;
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;
  q->cacheHits = 0;
  q->iteratorNode = NULL;
  q->currentNode = NULL;
  q->head = NULL;
  q->tail = NULL;
  q->freeHook = freeHook;
  logProcEnd ("");
  return q;
}

void
queueFree (queue_t *q)
{
  queuenode_t     *node;
  queuenode_t     *tnode;

  logProcBegin ();

  if (q == NULL) {
    logProcEnd ("null");
    return;
  }

  if (q->queueident != QUEUE_IDENT) {
    logProcEnd ("bad-ident");
    return;
  }

  /* make sure this queue is no longer available */
  q->queueident = 0;

  if (q->cacheHits > 0) {
    logMsg (LOG_DBG, LOG_BASIC, "queue: %s cache hits: %ld", q->name, q->cacheHits);
  }
  dataFree (q->name);
  node = q->head;
  tnode = node;
  while (node != NULL && node->next != NULL) {
    node = node->next;
    queueFreeNodeData (q, tnode);
    dataFree (tnode);
    tnode = node;
  }
  queueFreeNodeData (q, tnode);
  dataFree (tnode);
  mdfree (q);

  logProcEnd ("");
}

void
queuePush (queue_t *q, void *data)
{
  queuenode_t     *node;

  logProcBegin ();
  if (q == NULL) {
    logProcEnd ("bad-ptr");
    return;
  }
  if (q->queueident != QUEUE_IDENT) {
    logProcEnd ("bad-ident");
    return;
  }

  node = mdmalloc (sizeof (queuenode_t));
  node->prev = q->tail;
  node->next = NULL;
  node->data = data;

  if (q->head == NULL) {
    q->head = node;
  }
  if (node->prev != NULL) {
    node->prev->next = node;
  }
  q->tail = node;
  q->count++;

  /* a push does not invalidate the cache */
  /* the cache never contains the head nor the tail */

  logProcEnd ("");
}

void
queuePushHead (queue_t *q, void *data)
{
  queuenode_t     *node;

  logProcBegin ();
  if (q == NULL) {
    logProcEnd ("bad-ptr");
    return;
  }
  if (q->queueident != QUEUE_IDENT) {
    logProcEnd ("bad-ident");
    return;
  }

  node = mdmalloc (sizeof (queuenode_t));
  node->prev = NULL;
  node->next = q->head;
  node->data = data;

  if (q->tail == NULL) {
    q->tail = node;
  }
  if (node->next != NULL) {
    node->next->prev = node;
  }
  q->head = node;
  q->count++;

  /* push-head invalidates the cache */
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;

  logProcEnd ("");
}

void *
queueGetFirst (queue_t *q)
{
  void          *data = NULL;
  queuenode_t   *node;

  logProcBegin ();
  if (q == NULL) {
    logProcEnd ("bad-ptr");
    return NULL;
  }
  if (q->queueident != QUEUE_IDENT) {
    logProcEnd ("bad-ident");
    return NULL;
  }

  node = q->head;

  if (node != NULL) {
    data = node->data;
    /* there is no need to modify the cache */
  }

  logProcEnd ("");
  return data;
}

void *
queueGetByIdx (queue_t *q, qidx_t idx)
{
  queuenode_t       *node = NULL;
  void              *data = NULL;


  logProcBegin ();
  if (q == NULL) {
    logProcEnd ("bad-ptr");
    return NULL;
  }
  if (q->queueident != QUEUE_IDENT) {
    logProcEnd ("bad-ident");
    return NULL;
  }
  if (idx >= q->count) {
    logProcEnd ("bad-idx");
    return NULL;
  }

  node = queueGetNodeByIdx (q, idx);

  if (node != NULL) {
    data = node->data;
  }

  logProcEnd ("");
  return data;
}


void *
queuePop (queue_t *q)
{
  void          *data = NULL;
  queuenode_t   *node;

  logProcBegin ();
  if (q == NULL) {
    logProcEnd ("bad-ptr");
    return NULL;
  }
  if (q->queueident != QUEUE_IDENT) {
    logProcEnd ("bad-ident");
    return NULL;
  }
  node = q->head;

  /* a pop invalidates the cache */
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;

  data = queueRemove (q, node);
  logProcEnd ("");
  return data;
}

void
queueClear (queue_t *q, qidx_t startIdx)
{
  queuenode_t   *node;
  queuenode_t   *tnode;

  logProcBegin ();
  if (q == NULL) {
    logProcEnd ("bad-ptr");
    return;
  }
  if (q->queueident != QUEUE_IDENT) {
    logProcEnd ("bad-ident");
    return;
  }
  node = q->tail;

  while (node != NULL && q->count > startIdx) {
    tnode = node;
    node = node->prev;
    queueFreeNodeData (q, tnode);
    queueRemove (q, tnode);
  }

  /* clear invalidates the cache */
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;

  logProcEnd ("");
  return;
}

void
queueMove (queue_t *q, qidx_t fromidx, qidx_t toidx)
{
  queuenode_t   *fromnode = NULL;
  queuenode_t   *tonode = NULL;
  void          *tdata = NULL;

  logProcBegin ();
  if (q == NULL) {
    logProcEnd ("bad-ptr");
    return;
  }
  if (q->queueident != QUEUE_IDENT) {
    logProcEnd ("bad-ident");
    return;
  }
  if (fromidx < 0 || fromidx >= q->count || toidx < 0 || toidx >= q->count) {
    logProcEnd ("bad-idx");
    return;
  }

  fromnode = queueGetNodeByIdx (q, fromidx);
  tonode = queueGetNodeByIdx (q, toidx);

  if (fromnode != NULL && tonode != NULL) {
    /* messing with the pointers is a pain; simply swap the content pointers */
    tdata = fromnode->data;
    fromnode->data = tonode->data;
    tonode->data = tdata;
  }

  /* a move does not invalidate the cache */
  /* the cache points at a node, not the data */
  /* note that the cache is now pointing at to-node (if not head/tail) */

  logProcEnd ("");
  return;
}

void
queueInsert (queue_t *q, qidx_t idx, void *data)
{
  queuenode_t   *node = NULL;
  queuenode_t   *tnode = NULL;

  logProcBegin ();
  if (q == NULL) {
    logProcEnd ("bad-ptr");
    return;
  }
  if (q->queueident != QUEUE_IDENT) {
    logProcEnd ("bad-ident");
    return;
  }
  if (idx < 0 || idx >= q->count) {
    logProcEnd ("bad-idx");
    return;
  }

  node = mdmalloc (sizeof (queuenode_t));

  node->prev = NULL;
  node->next = NULL;
  node->data = data;

  tnode = queueGetNodeByIdx (q, idx);

  if (tnode != NULL) {
    node->prev = tnode->prev;
    node->next = tnode;
    tnode->prev = node;
    if (q->head == tnode) {
      q->head = node;
    }
  }
  if (tnode == NULL) {
    q->head = node;
    q->tail = node;
  }

  if (node->prev != NULL) {
    node->prev->next = node;
  }
  ++q->count;

  /* an insert resets the cache to point at the inserted node */
  q->cacheNode = node;
  q->cacheIdx = idx;

  logProcEnd ("");
  return;
}

void *
queueRemoveByIdx (queue_t *q, qidx_t idx)
{
  queuenode_t       *node = NULL;
  void              *data = NULL;

  logProcBegin ();
  if (q == NULL) {
    logProcEnd ("bad-ptr");
    return NULL;
  }
  if (q->queueident != QUEUE_IDENT) {
    logProcEnd ("bad-ident");
    return NULL;
  }
  if (idx < 0 || idx >= q->count) {
    logProcEnd ("bad-idx");
    return NULL;
  }

  node = queueGetNodeByIdx (q, idx);

  if (node != NULL) {
    /* queueRemove will invalidate the cache */
    data = queueRemove (q, node);
  }

  logProcEnd ("");
  return data;
}

qidx_t
queueGetCount (queue_t *q)
{
  if (q == NULL) {
    return 0;
  }
  if (q->queueident != QUEUE_IDENT) {
    return 0;
  }

  return q->count;
}

void
queueStartIterator (queue_t *q, qidx_t *iteridx)
{
  if (q == NULL) {
    return;
  }
  if (q->queueident != QUEUE_IDENT) {
    return;
  }

  *iteridx = -1;
  q->iteratorNode = q->head;
  q->currentNode = q->head;
}

void *
queueIterateData (queue_t *q, qidx_t *iteridx)
{
  void      *data = NULL;

  if (q == NULL) {
    return NULL;
  }
  if (q->queueident != QUEUE_IDENT) {
    return NULL;
  }

  if (q->iteratorNode != NULL) {
    data = q->iteratorNode->data;
    ++(*iteridx);
    q->currentNode = q->iteratorNode;
    q->iteratorNode = q->iteratorNode->next;

    /* put the fetched node into the cache */
    q->cacheNode = q->currentNode;
    q->cacheIdx = *iteridx;
  }
  return data;
}

void *
queueIterateRemoveNode (queue_t *q, qidx_t *iteridx)
{
  void        *data = NULL;

  if (q == NULL) {
    return NULL;
  }
  if (q->queueident != QUEUE_IDENT) {
    return NULL;
  }

  /* queueRemove will invalidate the cache */
  data = queueRemove (q, q->currentNode);
  q->currentNode = q->iteratorNode;

  return data;
}

int
queueDebugSearchDist (queue_t *q)
{
  if (q == NULL) {
    return -1;
  }
  if (q->queueident != QUEUE_IDENT) {
    return -1;
  }

  return q->searchDist;
}

/* internal routines */

static queuenode_t *
queueGetNodeByIdx (queue_t *q, qidx_t idx)
{
  qidx_t            count = 0;
  queuenode_t       *node = NULL;
  int               dist;
  int               dir;

  /* some minor optimization to make by-idx more efficient */
  q->searchDist = 0;
  dist = idx;
  dir = Q_DIR_NEXT;
  node = q->head;

  if (abs ((q->count - 1) - idx) < dist) {
    dist = abs ((q->count - 1) - idx);
    dir = Q_DIR_PREV;
    node = q->tail;
    count = q->count - 1;
  }

  if (q->cacheNode != NULL) {
    if (idx == q->cacheIdx) {
      q->cacheHits++;
    }
    if (abs (q->cacheIdx - idx) < dist) {
      dist = abs (q->cacheIdx - idx);
      count = q->cacheIdx;
      node = q->cacheNode;
      if (idx < q->cacheIdx) {
        dir = Q_DIR_PREV;
      } else {
        dir = Q_DIR_NEXT;
      }
    }
  }

  while (node != NULL && count != idx) {
    count += dir;
    if (dir == Q_DIR_PREV) {
      node = node->prev;
    } else {
      node = node->next;
    }
    ++q->searchDist;
  }

  /* there is no need to modify the cache if indexing the head or tail */
  /* keep whatever the cache was pointing at before */
  if (idx != 0 && idx != q->count - 1) {
    q->cacheIdx = idx;
    q->cacheNode = node;
  }

  return node;
}

static void *
queueRemove (queue_t *q, queuenode_t *node)
{
  void          *data = NULL;

  if (node == NULL) {
    return NULL;
  }
  if (q == NULL) {
    return NULL;
  }
  if (q->head == NULL) {
    return NULL;
  }

  data = node->data;
  if (node->prev == NULL) {
    q->head = node->next;
  } else {
    node->prev->next = node->next;
  }

  if (node->next == NULL) {
    q->tail = node->prev;
  } else {
    node->next->prev = node->prev;
  }
  q->count--;
  /* the data is being returned, do not free it */
  mdfree (node);

  /* a removal invalidates the cache */
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;

  return data;
}

static void
queueFreeNodeData (queue_t *q, queuenode_t *node)
{
  if (node == NULL) {
    return;
  }
  if (node->data != NULL && q->freeHook != NULL) {
    q->freeHook (node->data);
    node->data = NULL;
  }
}

