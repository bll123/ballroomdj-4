#ifndef INC_QUEUE_H
#define INC_QUEUE_H

typedef void (*queueFree_t)(void *);

typedef struct queuenode queuenode_t;

typedef struct queue queue_t;

queue_t *queueAlloc (queueFree_t freeHook);
void    queueFree (queue_t *q);
void    queuePush (queue_t *q, void *data);
void    queuePushHead (queue_t *q, void *data);
void    *queueGetCurrent (queue_t *q);
void    *queueGetByIdx (queue_t *q, ssize_t idx);
void    *queuePop  (queue_t *q);
void    queueClear (queue_t *q, ssize_t startIdx);
void    queueMove (queue_t *q, ssize_t fromIdx, ssize_t toIdx);
void    queueInsert (queue_t *q, ssize_t idx, void *data);
void    *queueRemoveByIdx (queue_t *q, ssize_t idx);
ssize_t queueGetCount (queue_t *q);
void    queueStartIterator (queue_t *q, ssize_t *idx);
void    *queueIterateData (queue_t *q, ssize_t *idx);
void    *queueIterateRemoveNode (queue_t *q, ssize_t *idx);

#endif /* INC_QUEUE_H */
