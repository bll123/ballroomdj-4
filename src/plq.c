#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "plq.h"
#include "queue.h"
#include "playlist.h"

static void plqQueueFree (void *tq);

plq_t *
plqAlloc (void)
{
  plq_t *plq = malloc (sizeof (plq_t));
  assert (plq != NULL);
  plq->q = queueAlloc (plqQueueFree);
  return plq;
}

void
plqFree (plq_t *plq)
{
  if (plq != NULL) {
    if (plq->q != NULL) {

      queueFree (plq->q);
    }
    free (plq);
  }
}

void
plqPush (plq_t *plq, char *plname)
{
  plqitem_t     *plqitem;

  plqitem = malloc (sizeof (plqitem_t));
  assert (plqitem != NULL);
  plqitem->playlist = playlistAlloc (plname);
  queuePush (plq->q, plqitem);
}

static void
plqQueueFree (void *titem)
{
  plqitem_t   *plqitem = titem;

  if (plqitem != NULL) {
    if (plqitem->playlist != NULL) {
      playlistFree (plqitem->playlist);
    }
    free (plqitem);
  }
}

playlist_t *
plqGetCurrent (plq_t *plq)
{
  plqitem_t *plqitem = queueGetCurrent (plq->q);
  return plqitem->playlist;
}
