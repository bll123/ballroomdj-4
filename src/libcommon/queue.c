#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "inline.h"
#include "log.h"
#include "queue.h"

enum {
  QUEUE_NO_IDX = -1,
};

typedef struct queuenode {
  void              *data;
  struct queuenode  *prev;
  struct queuenode  *next;
} queuenode_t;

typedef struct queue {
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
} queue_t;

static void * queueRemove (queue_t *q, queuenode_t *node);

queue_t *
queueAlloc (const char *name, queueFree_t freeHook)
{
  queue_t     *q;

  logProcBegin (LOG_PROC, "queueAlloc");
  q = malloc (sizeof (queue_t));
  assert (q != NULL);
  q->name = strdup (name);
  q->count = 0;
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;
  q->cacheHits = 0;
  q->iteratorNode = NULL;
  q->currentNode = NULL;
  q->head = NULL;
  q->tail = NULL;
  q->freeHook = freeHook;
  logProcEnd (LOG_PROC, "queueAlloc", "");
  return q;
}

void
queueFree (queue_t *q)
{
  queuenode_t     *node;
  queuenode_t     *tnode;

  logProcBegin (LOG_PROC, "queueFree");
  if (q != NULL) {
    if (q->cacheHits > 0) {
      logMsg (LOG_DBG, LOG_BASIC, "queue: %s cache hits: %ld", q->name, q->cacheHits);
    }
    dataFree (q->name);
    node = q->head;
    tnode = node;
    while (node != NULL && node->next != NULL) {
      node = node->next;
      if (tnode != NULL) {
        if (tnode->data != NULL && q->freeHook != NULL) {
          q->freeHook (tnode->data);
        }
        free (tnode);
      }
      tnode = node;
    }
    if (tnode != NULL) {
      if (tnode->data != NULL && q->freeHook != NULL) {
        q->freeHook (tnode->data);
      }
      free (tnode);
    }
    free (q);
  }
  logProcEnd (LOG_PROC, "queueFree", "");
}

void
queuePush (queue_t *q, void *data)
{
  queuenode_t     *node;

  logProcBegin (LOG_PROC, "queuePush");
  if (q == NULL) {
    logProcEnd (LOG_PROC, "queuePush", "bad-ptr");
    return;
  }

  node = malloc (sizeof (queuenode_t));
  assert (node != NULL);
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
  logProcEnd (LOG_PROC, "queuePush", "");
}

void
queuePushHead (queue_t *q, void *data)
{
  queuenode_t     *node;

  logProcBegin (LOG_PROC, "queuePushHead");
  if (q == NULL) {
    logProcEnd (LOG_PROC, "queuePushHead", "bad-ptr");
    return;
  }

  node = malloc (sizeof (queuenode_t));
  assert (node != NULL);
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

  logProcEnd (LOG_PROC, "queuePushHead", "");
}

void *
queueGetCurrent (queue_t *q)
{
  void          *data = NULL;
  queuenode_t   *node;

  logProcBegin (LOG_PROC, "queueGetCurrent");
  if (q == NULL) {
    logProcEnd (LOG_PROC, "queueGetCurrent", "bad-ptr");
    return NULL;
  }
  node = q->head;

  if (node != NULL) {
    data = node->data;
  }
  logProcEnd (LOG_PROC, "queueGetCurrent", "");
  return data;
}

void *
queueGetByIdx (queue_t *q, qidx_t idx)
{
  qidx_t            count = 0;
  queuenode_t       *node = NULL;
  void              *data = NULL;

  logProcBegin (LOG_PROC, "queueGetByIdx");
  if (q == NULL) {
    logProcEnd (LOG_PROC, "queueGetByIdx", "bad-ptr");
    return NULL;
  }
  if (idx >= q->count) {
    logProcEnd (LOG_PROC, "queueGetByIdx", "bad-idx");
    return NULL;
  }

  if (idx == q->cacheIdx && q->cacheNode != NULL) {
    data = q->cacheNode->data;
    q->cacheHits++;
    logProcEnd (LOG_PROC, "queueGetByIdx", "from-cache");
    return data;
  }

  node = q->head;
  while (node != NULL && count != idx) {
    ++count;
    node = node->next;
  }
  if (node != NULL) {
    q->cacheIdx = idx;
    q->cacheNode = node;
    data = node->data;
  }
  logProcEnd (LOG_PROC, "queueGetByIdx", "");
  return data;
}


void *
queuePop (queue_t *q)
{
  void          *data = NULL;
  queuenode_t   *node;

  logProcBegin (LOG_PROC, "queuePop");
  if (q == NULL) {
    logProcEnd (LOG_PROC, "queuePop", "bad-ptr");
    return NULL;
  }
  node = q->head;

  /* a pop invalidates the cache */
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;

  data = queueRemove (q, node);
  logProcEnd (LOG_PROC, "queuePop", "");
  return data;
}

void
queueClear (queue_t *q, qidx_t startIdx)
{
  queuenode_t   *node;
  queuenode_t   *tnode;

  logProcBegin (LOG_PROC, "queueClear");
  if (q == NULL) {
    logProcEnd (LOG_PROC, "queueClear", "bad-ptr");
    return;
  }
  node = q->tail;

  while (node != NULL && q->count > startIdx) {
    tnode = node;
    node = node->prev;
    if (tnode->data != NULL && q->freeHook != NULL) {
      q->freeHook (tnode->data);
    }
    queueRemove (q, tnode);
  }

  /* invalidate the cache */
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;

  logProcEnd (LOG_PROC, "queueClear", "");
  return;
}

void
queueMove (queue_t *q, qidx_t fromidx, qidx_t toidx)
{
  queuenode_t   *node = NULL;
  queuenode_t   *fromnode = NULL;
  queuenode_t   *tonode = NULL;
  void          *tdata = NULL;
  qidx_t        count;

  logProcBegin (LOG_PROC, "queueMove");
  if (q == NULL) {
    logProcEnd (LOG_PROC, "queueMove", "bad-ptr");
    return;
  }
  if (fromidx < 0 || fromidx >= q->count || toidx < 0 || toidx >= q->count) {
    logProcEnd (LOG_PROC, "queueMove", "bad-idx");
    return;
  }
  node = q->head;

  count = 0;
  while (node != NULL && (fromnode == NULL || tonode == NULL)) {
    if (count == fromidx) {
      fromnode = node;
    }
    if (count == toidx) {
      tonode = node;
    }
    node = node->next;
    ++count;
  }

  if (fromnode != NULL && tonode != NULL) {
    /* messing with the pointers is a pain; simply swap the content pointers */
    tdata = fromnode->data;
    fromnode->data = tonode->data;
    tonode->data = tdata;
  }

  /* a move invalidates the cache */
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;

  logProcEnd (LOG_PROC, "queueMove", "");
  return;
}

void
queueInsert (queue_t *q, qidx_t idx, void *data)
{
  queuenode_t   *node = NULL;
  queuenode_t   *tnode = NULL;
  qidx_t        count;

  logProcBegin (LOG_PROC, "queueInsert");
  if (q == NULL) {
    logProcEnd (LOG_PROC, "queueInsert", "bad-ptr");
    return;
  }
  if (idx < 0 || idx >= q->count) {
    logProcEnd (LOG_PROC, "queueInsert", "bad-idx");
    return;
  }

  node = malloc (sizeof (queuenode_t));
  assert (node != NULL);
  node->prev = NULL;
  node->next = NULL;
  node->data = data;

  count = 0;
  tnode = q->head;
  while (tnode != NULL && count != idx) {
    tnode = tnode->next;
    ++count;
  }

  if (tnode != NULL) {
    node->prev = tnode->prev;
    node->next = tnode;
    tnode->prev = node;
    if (q->head == tnode) {
      q->head = node;
    }
  } else {
    q->head = node;
    q->tail = node;
  }
  if (node->prev != NULL) {
    node->prev->next = node;
  }
  ++q->count;

  /* an insert invalidates the cache */
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;

  logProcEnd (LOG_PROC, "queueInsert", "");
  return;
}

void *
queueRemoveByIdx (queue_t *q, qidx_t idx)
{
  qidx_t            count = 0;
  queuenode_t       *node = NULL;
  void              *data = NULL;

  logProcBegin (LOG_PROC, "queueRemoveByIdx");
  if (q == NULL) {
    logProcEnd (LOG_PROC, "queueRemoveByIdx", "bad-ptr");
    return NULL;
  }
  if (idx < 0 || idx >= q->count) {
    logProcEnd (LOG_PROC, "queueRemoveByIdx", "bad-idx");
    return NULL;
  }
  node = q->head;
  while (node != NULL && count != idx) {
    ++count;
    node = node->next;
  }
  if (node != NULL) {
    data = queueRemove (q, node);
  }

  /* a removal invalidates the cache */
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;

  logProcEnd (LOG_PROC, "queueRemoveByIdx", "");
  return data;
}

qidx_t
queueGetCount (queue_t *q)
{
  if (q != NULL) {
    return q->count;
  }
  return 0;
}

void
queueStartIterator (queue_t *q, qidx_t *iteridx)
{
  if (q != NULL) {
    *iteridx = -1;
    q->iteratorNode = q->head;
    q->currentNode = q->head;
  }
}

void *
queueIterateData (queue_t *q, qidx_t *iteridx)
{
  void      *data = NULL;

  if (q->iteratorNode != NULL) {
    data = q->iteratorNode->data;
    ++(*iteridx);
    q->currentNode = q->iteratorNode;
    q->iteratorNode = q->iteratorNode->next;
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
  data = queueRemove (q, q->currentNode);
  q->currentNode = q->iteratorNode;

  /* a removal invalidates the cache */
  q->cacheNode = NULL;
  q->cacheIdx = QUEUE_NO_IDX;

  return data;
}

/* internal routines */

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
  free (node);
  return data;
}

