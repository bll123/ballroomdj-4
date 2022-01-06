#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "musicq.h"
#include "queue.h"

static void musicqQueueFree (void *tq);

musicq_t *
musicqAlloc (void)
{
  musicq_t *musicq = malloc (sizeof (musicq_t));
  assert (musicq != NULL);
  musicq->q = queueAlloc (musicqQueueFree);
  return musicq;
}

void
musicqFree (musicq_t *musicq)
{
  if (musicq != NULL) {
    queueFree (musicq->q);
    free (musicq);
  }
}

void
musicqQueueFree (void *tq)
{
  ;
}
