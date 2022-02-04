#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "bdjvarsdf.h"
#include "dance.h"
#include "musicq.h"
#include "queue.h"
#include "tagdef.h"

static void musicqQueueItemFree (void *tqitem);
static int  musicqRenumberStart (musicq_t *musicq, musicqidx_t musicqidx);
static void musicqRenumber (musicq_t *musicq, musicqidx_t musicqidx, int olddispidx);

musicq_t *
musicqAlloc (void)
{
  musicq_t *musicq = malloc (sizeof (musicq_t));
  assert (musicq != NULL);
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    musicq->q [i] = queueAlloc (musicqQueueItemFree);
    musicq->uniqueidx [i] = 0;
    musicq->dispidx [i] = 1;
  }
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
musicqPush (musicq_t *musicq, musicqidx_t musicqidx, song_t *song, char *plname)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL || song == NULL) {
    return;
  }

  musicqitem = malloc (sizeof (musicqitem_t));
  assert (musicqitem != NULL);
  musicqitem->dispidx = musicq->dispidx [musicqidx];
  ++(musicq->dispidx [musicqidx]);
  musicqitem->uniqueidx = musicq->uniqueidx [musicqidx];
  ++(musicq->uniqueidx [musicqidx]);
  musicqitem->song = song;
  musicqitem->playlistName = strdup (plname);
  musicqitem->announce = NULL;
  musicqitem->flags = MUSICQ_FLAG_NONE;
  queuePush (musicq->q [musicqidx], musicqitem);
}

void
musicqPushHeadEmpty (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return;
  }

  musicqitem = malloc (sizeof (musicqitem_t));
  assert (musicqitem != NULL);
  musicqitem->dispidx = 0;
  musicqitem->uniqueidx = musicq->uniqueidx [musicqidx];
  ++(musicq->uniqueidx [musicqidx]);
  musicqitem->song = NULL;
  musicqitem->playlistName = NULL;
  musicqitem->announce = NULL;
  musicqitem->flags = MUSICQ_FLAG_EMPTY;
  queuePushHead (musicq->q [musicqidx], musicqitem);
}

void
musicqMove (musicq_t *musicq, musicqidx_t musicqidx,
    ssize_t fromidx, ssize_t toidx)
{
  int   olddispidx;


  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueMove (musicq->q [musicqidx], fromidx, toidx);
  musicqRenumber (musicq, musicqidx, olddispidx);
}

void
musicqInsert (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx, song_t *song)
{
  int           olddispidx;
  musicqitem_t  *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL || song == NULL) {
    return;
  }
  if (idx < 1 || idx >= queueGetCount (musicq->q [musicqidx])) {
    return;
  }

  olddispidx = musicqRenumberStart (musicq, musicqidx);

  musicqitem = malloc (sizeof (musicqitem_t));
  assert (musicqitem != NULL);
  musicqitem->song = song;
  musicqitem->playlistName = NULL;
  musicqitem->announce = NULL;
  musicqitem->flags = MUSICQ_FLAG_REQUEST;
  musicqitem->uniqueidx = musicq->uniqueidx [musicqidx];
  ++musicq->uniqueidx [musicqidx];

  queueInsert (musicq->q [musicqidx], idx, musicqitem);
  musicqRenumber (musicq, musicqidx, olddispidx);
}

song_t *
musicqGetCurrent (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return NULL;
  }

  musicqitem = queueGetCurrent (musicq->q [musicqidx]);
  if (musicqitem == NULL) {
    return NULL;
  }
  return musicqitem->song;
}

/* gets by the real idx, not the display index */
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

int
musicqGetDispIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return MUSICQ_FLAG_NONE;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    return musicqitem->dispidx;
  }
  return -1;
}

int
musicqGetUniqueIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return MUSICQ_FLAG_NONE;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    return musicqitem->uniqueidx;
  }
  return -1;
}

void
musicqSetFlag (musicq_t *musicq, musicqidx_t musicqidx,
    ssize_t qkey, musicqflag_t flags)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    musicqitem->flags |= flags;
  }
  return;
}

void
musicqClearFlag (musicq_t *musicq, musicqidx_t musicqidx,
    ssize_t qkey, musicqflag_t flags)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    musicqitem->flags &= ~flags;
  }
  return;
}

char *
musicqGetAnnounce (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return NULL;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    return musicqitem->announce;
  }
  return NULL;
}

void
musicqSetAnnounce (musicq_t *musicq, musicqidx_t musicqidx,
    ssize_t qkey, char *annfname)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    musicqitem->announce = strdup (annfname);
  }
  return;
}

char *
musicqGetPlaylistName (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return NULL;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    return musicqitem->playlistName;
  }
  return NULL;
}

void
musicqPop (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return;
  }

  musicqitem = queuePop (musicq->q [musicqidx]);
  musicqQueueItemFree (musicqitem);
}

/* does not clear the initial entry -- that's the song that is playing */
void
musicqClear (musicq_t *musicq, musicqidx_t musicqidx, ssize_t startIdx)
{
  int       olddispidx;

  if (startIdx < 1 || startIdx >= queueGetCount (musicq->q [musicqidx])) {
    return;
  }

  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueClear (musicq->q [musicqidx], startIdx);
  musicq->dispidx [musicqidx] = olddispidx + queueGetCount (musicq->q [musicqidx]);
}

void
musicqRemove (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx)
{
  int       olddispidx;

  if (idx < 1 || idx >= queueGetCount (musicq->q [musicqidx])) {
    return;
  }

  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueRemoveByIdx (musicq->q [musicqidx], idx);
  musicqRenumber (musicq, musicqidx, olddispidx);
  --musicq->dispidx [musicqidx];
}

ssize_t
musicqGetLen (musicq_t *musicq, musicqidx_t musicqidx)
{
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return 0;
  }

  return queueGetCount (musicq->q [musicqidx]);
}


char *
musicqGetData (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx, tagdefkey_t tagidx)
{
  musicqitem_t  *musicqitem;
  song_t        *song;
  char          *data = NULL;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return NULL;
  }
  if (idx >= queueGetCount (musicq->q [musicqidx])) {
    return NULL;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], idx);
  if (musicqitem != NULL &&
     (musicqitem->flags & MUSICQ_FLAG_EMPTY) != MUSICQ_FLAG_EMPTY) {
    song = musicqitem->song;
    data = songGetData (song, tagidx);
  }
  return data;
}

char *
musicqGetDance (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx)
{
  musicqitem_t  *musicqitem = NULL;
  song_t        *song = NULL;
  listidx_t     dancekey;
  char          *danceStr = NULL;
  dance_t       *dances = NULL;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return NULL;
  }
  if (idx >= queueGetCount (musicq->q [musicqidx])) {
    return NULL;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], idx);
  if (musicqitem != NULL &&
     (musicqitem->flags & MUSICQ_FLAG_EMPTY) != MUSICQ_FLAG_EMPTY) {
    song = musicqitem->song;
    dancekey = songGetNum (song, TAG_DANCE);
    dances = bdjvarsdf [BDJVDF_DANCES];
    danceStr = danceGetData (dances, dancekey, DANCE_DANCE);
  }
  return danceStr;
}

static void
musicqQueueItemFree (void *titem)
{
  musicqitem_t        *musicqitem = titem;

  if (musicqitem != NULL) {
    if (musicqitem->playlistName != NULL) {
      free (musicqitem->playlistName);
    }
    if (musicqitem->announce != NULL) {
      free (musicqitem->announce);
    }
    free (musicqitem);
  }
}

static int
musicqRenumberStart (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t  *musicqitem;
  int           dispidx = -1;

  musicqitem = queueGetByIdx (musicq->q [musicqidx], 0);
  if (musicqitem != NULL) {
    dispidx = musicqitem->dispidx;
  }
  return dispidx;
}

static void
musicqRenumber (musicq_t *musicq, musicqidx_t musicqidx, int olddispidx)
{
  ssize_t       count;
  int           dispidx = olddispidx;
  ssize_t       iteridx;
  musicqitem_t  *musicqitem;

  count = queueGetCount (musicq->q [musicqidx]);
  queueStartIterator (musicq->q [musicqidx], &iteridx);
  while ((musicqitem = queueIterateData (musicq->q [musicqidx], &iteridx)) != NULL) {
    musicqitem->dispidx = dispidx;
    ++dispidx;
  }
}
