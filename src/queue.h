#ifndef INC_QUEUE_H
#define INC_QUEUE_H

typedef struct queuenode {
  void              *data;
  struct queuenode  *next;
} queuenode_t;

typedef struct {
  long          count;
  long          iteratorIndex;
  long          currentIndex;
  queuenode_t   *iteratorNode;
  queuenode_t   *currentNode;
  queuenode_t   root;
  queuenode_t   *tail;
} queue_t;

queue_t *queueAlloc (void);
void    queueFree (queue_t *q);
void    queuePush (queue_t *q, void *data);
void    *queuePop  (queue_t *q);
void    queueRemove (queue_t *q, long idx, void *data);
long    queueGetCount (queue_t *q);
void    queueStartIterator (queue_t *q);
void    *queueIterateData (queue_t *q);

#endif /* INC_QUEUE_H */
