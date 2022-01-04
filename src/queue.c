#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "queue.h"

queue_t *
queueAlloc (void)
{
  queue_t     *q;

  q = malloc (sizeof (queue_t));
  assert (q != NULL);
  q->count = 0;
  q->iteratorIndex = 0;
  q->currentIndex = 0;
  q->iteratorNode = NULL;
  q->currentNode = NULL;
  q->root.data = NULL;
  q->root.next = NULL;
  q->tail = &q->root;
  return q;
}

void
queueFree (queue_t *q)
{
  queuenode_t     *node;
  queuenode_t     *tnode;

  if (q != NULL) {
    node = &q->root;
    tnode = NULL;
    while (node->next != NULL) {
      node = node->next;
      if (tnode != NULL) {
        free (tnode);
      }
      tnode = node;
    }
    if (tnode != NULL) {
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

  node->next = NULL;
  node->data = data;

  q->tail->next = node;
  q->tail = node;
  q->count++;
}

void *
queuePop (queue_t *q)
{
  void          *data;
  queuenode_t   *node;

  if (q == NULL) {
    return NULL;
  }
  if (q->root.next == NULL) {
    return NULL;
  }
  node = q->root.next;
  data = node->data;
  q->root.next = node->next;
  if (q->root.next == NULL) {
    q->tail = &q->root;
  }
  q->count--;
  free (node);
  return data;
}

void
queueRemove (queue_t *q, long idx, void *data)
{
  return;
}

long
queueGetCount (queue_t *q)
{
  if (q != NULL) {
    return q->count;
  }
  return 0;
}

void
queueStartIterator (queue_t *q)
{
  if (q != NULL) {
    q->iteratorIndex = 0;
    q->currentIndex = 0;
    q->iteratorNode = q->root.next;
    q->currentNode = q->root.next;
  }
}

void *
queueIterateData (queue_t *q)
{
  void      *data;

  if (q->iteratorNode != NULL) {
    data = q->iteratorNode->data;
    q->currentIndex++;
    q->currentNode = q->iteratorNode;
    q->iteratorNode = q->iteratorNode->next;
    return data;
  }
  return NULL;
}
