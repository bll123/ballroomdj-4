/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_QUEUE_H
#define INC_QUEUE_H

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef int32_t qidx_t;

typedef void (*queueFree_t)(void *);

typedef struct queuenode queuenode_t;
typedef struct queue queue_t;

queue_t *queueAlloc (const char *name, queueFree_t freeHook);
void    queueFree (queue_t *q);
void    queuePush (queue_t *q, void *data);
void    queuePushHead (queue_t *q, void *data);
void    *queueGetFirst (queue_t *q);
void    *queueGetByIdx (queue_t *q, qidx_t idx);
void    *queuePop  (queue_t *q);
void    queueClear (queue_t *q, qidx_t startIdx);
void    queueMove (queue_t *q, qidx_t fromIdx, qidx_t toIdx);
void    queueInsert (queue_t *q, qidx_t idx, void *data);
void    *queueRemoveByIdx (queue_t *q, qidx_t idx);
qidx_t  queueGetCount (queue_t *q);
void    queueStartIterator (queue_t *q, qidx_t *idx);
void    *queueIterateData (queue_t *q, qidx_t *idx);
void    *queueIterateRemoveNode (queue_t *q, qidx_t *idx);
int     queueDebugSearchDist (queue_t *q);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_QUEUE_H */
