#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "queue.h"

static void * queueRemove (queue_t *q, queuenode_t *node);

queue_t *
queueAlloc (queueFree_t freeHook)
{
  queue_t     *q;

  q = malloc (sizeof (queue_t));
  assert (q != NULL);
  q->count = 0;
  q->iteratorNode = NULL;
  q->currentNode = NULL;
  q->head = NULL;
  q->tail = NULL;
  q->freeHook = freeHook;
  return q;
}

void
queueFree (queue_t *q)
{
  queuenode_t     *node;
  queuenode_t     *tnode;

  if (q != NULL) {
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
}

void
queuePush (queue_t *q, void *data)
{
  queuenode_t     *node;

  if (q == NULL) {
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
}

void
queuePushHead (queue_t *q, void *data)
{
  queuenode_t     *node;

  if (q == NULL) {
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
}

void *
queueGetCurrent (queue_t *q)
{
  void          *data = NULL;
  queuenode_t   *node;

  if (q == NULL) {
    return NULL;
  }
  node = q->head;

  if (node != NULL) {
    data = node->data;
  }
  return data;
}

void *
queueGetByIdx (queue_t *q, ssize_t idx)
{
  ssize_t           count = 0;
  queuenode_t       *node = NULL;
  void              *data = NULL;

  if (q == NULL) {
    return NULL;
  }
  if (idx >= q->count) {
    return NULL;
  }
  node = q->head;
  while (node != NULL && count != idx) {
    ++count;
    node = node->next;
  }
  if (node != NULL) {
    data = node->data;
  }
  return data;
}


void *
queuePop (queue_t *q)
{
  void          *data;
  queuenode_t   *node;

  if (q == NULL) {
    return NULL;
  }
  node = q->head;

  data = queueRemove (q, node);
  return data;
}

void
queueClear (queue_t *q, ssize_t startIdx)
{
  queuenode_t   *node;
  queuenode_t   *tnode;

  if (q == NULL) {
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
  return;
}

void
queueMove (queue_t *q, ssize_t fromidx, ssize_t toidx)
{
  queuenode_t   *node = NULL;
  queuenode_t   *fromnode = NULL;
  queuenode_t   *tonode = NULL;
  void          *tdata = NULL;
  ssize_t       count;

  if (q == NULL) {
    return;
  }
  if (fromidx < 0 || fromidx >= q->count || toidx < 0 || toidx >= q->count) {
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

  return;
}

void
queueInsert (queue_t *q, ssize_t idx, void *data)
{
  queuenode_t   *node = NULL;
  queuenode_t   *tnode = NULL;
  ssize_t       count;

  if (q == NULL) {
    return;
  }
  if (idx < 0 || idx >= q->count) {
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

  return;
}

void *
queueRemoveByIdx (queue_t *q, ssize_t idx)
{
  ssize_t           count = 0;
  queuenode_t       *node = NULL;
  void              *data = NULL;

  if (q == NULL) {
    return NULL;
  }
  if (idx < 0 || idx >= q->count) {
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
  return data;
}

ssize_t
queueGetCount (queue_t *q)
{
  if (q != NULL) {
    return q->count;
  }
  return 0;
}

void
queueStartIterator (queue_t *q, ssize_t *iteridx)
{
  if (q != NULL) {
    *iteridx = -1;
    q->iteratorNode = q->head;
    q->currentNode = q->head;
  }
}

void *
queueIterateData (queue_t *q, ssize_t *iteridx)
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
queueIterateRemoveNode (queue_t *q, ssize_t *iteridx)
{
  void        *data = NULL;

  if (q == NULL) {
    return NULL;
  }
  data = queueRemove (q, q->currentNode);
  q->currentNode = q->iteratorNode;
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

