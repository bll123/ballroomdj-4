#ifndef INC_QUEUE_H
#define INC_QUEUE_H

typedef void (*queueFree_t)(void *);

typedef struct queuenode {
  void              *data;
  struct queuenode  *prev;
  struct queuenode  *next;
} queuenode_t;

typedef struct {
  long          count;
  long          currentIndex;
  queuenode_t   *iteratorNode;
  queuenode_t   *currentNode;
  queuenode_t   *head;
  queuenode_t   *tail;
  queueFree_t   freeHook;
} queue_t;

queue_t *queueAlloc (queueFree_t freeHook);
void    queueFree (queue_t *q);
void    queuePush (queue_t *q, void *data);
void    *queuePop  (queue_t *q);
void    *queueRemoveByIdx (queue_t *q, long idx);
long    queueGetCount (queue_t *q);
void    queueStartIterator (queue_t *q);
void    *queueIterateData (queue_t *q);
void    *queueIterateRemoveNode (queue_t *q);

#endif /* INC_QUEUE_H */
