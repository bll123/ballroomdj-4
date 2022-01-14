#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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
  musicq->q [MUSICQ_A] = queueAlloc (musicqQueueFree);
  musicq->q [MUSICQ_B] = queueAlloc (musicqQueueFree);
  return musicq;
}

void
musicqFree (musicq_t *musicq)
{
  if (musicq != NULL) {
    if (musicq->q [MUSICQ_A] != NULL) {
      queueFree (musicq->q [MUSICQ_A]);
    }
    if (musicq->q [MUSICQ_B] != NULL) {
      queueFree (musicq->q [MUSICQ_B]);
    }
    free (musicq);
  }
}

void
musicqQueueFree (void *titem)
{
  musicqitem_t        *musicqitem = titem;

  if (musicqitem != NULL) {
    free (musicqitem);
  }
}

void
musicqPush (musicq_t *musicq, musicqidx_t idx, song_t *song)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [idx] == NULL) {
    return;
  }

  musicqitem = malloc (sizeof (musicqitem_t));
  assert (musicqitem != NULL);
  musicqitem->song = song;
  musicqitem->flags = MUSICQ_FLAG_NONE;
  queuePush (musicq->q [idx], musicqitem);
}

song_t *
musicqGetCurrent (musicq_t *musicq, musicqidx_t idx)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [idx] == NULL) {
    return NULL;
  }

  musicqitem = queueGetCurrent (musicq->q [idx]);
  if (musicqitem == NULL) {
    return NULL;
  }
  return musicqitem->song;
}

song_t *
musicqGetByIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return NULL;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    return musicqitem->song;
  }
  return NULL;
}

musicqflag_t
musicqGetFlags (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return MUSICQ_FLAG_NONE;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    return musicqitem->flags;
  }
  return MUSICQ_FLAG_NONE;
}

void
musicqSetFlags (musicq_t *musicq, musicqidx_t musicqidx,
    ssize_t qkey, musicqflag_t flags)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    musicqitem->flags = flags;
  }
  return;
}

void
musicqPop (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return;
  }

  musicqitem = queuePop (musicq->q [musicqidx]);
  musicqQueueFree (musicqitem);
}

ssize_t
musicqGetLen (musicq_t *musicq, musicqidx_t musicqidx)
{
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return 0;
  }

  return queueGetCount (musicq->q [musicqidx]);
}
