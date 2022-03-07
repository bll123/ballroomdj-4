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
#include "log.h"
#include "queue.h"
#include "tagdef.h"

static void musicqQueueItemFree (void *tqitem);
static int  musicqRenumberStart (musicq_t *musicq, musicqidx_t musicqidx);
static void musicqRenumber (musicq_t *musicq, musicqidx_t musicqidx, int olddispidx);

musicq_t *
musicqAlloc (void)
{
  musicq_t  *musicq;

  logProcBegin (LOG_PROC, "musicqAlloc");
  musicq = malloc (sizeof (musicq_t));
  assert (musicq != NULL);
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    musicq->q [i] = queueAlloc (musicqQueueItemFree);
    musicq->uniqueidx [i] = 0;
    musicq->dispidx [i] = 1;
    musicq->duration [i] = 0;
  }
  logProcEnd (LOG_PROC, "musicqAlloc", "");
  return musicq;
}

void
musicqFree (musicq_t *musicq)
{
  logProcBegin (LOG_PROC, "musicqFree");
  if (musicq != NULL) {
    if (musicq->q [MUSICQ_A] != NULL) {
      queueFree (musicq->q [MUSICQ_A]);
    }
    if (musicq->q [MUSICQ_B] != NULL) {
      queueFree (musicq->q [MUSICQ_B]);
    }
    free (musicq);
  }
  logProcEnd (LOG_PROC, "musicqFree", "");
}

void
musicqPush (musicq_t *musicq, musicqidx_t musicqidx, song_t *song, char *plname)
{
  musicqitem_t      *musicqitem;
  ssize_t           dur;

  logProcBegin (LOG_PROC, "musicqPush");
  if (musicq == NULL || musicq->q [musicqidx] == NULL || song == NULL) {
    logProcEnd (LOG_PROC, "musicqPush", "bad-ptr");
    return;
  }

  musicqitem = malloc (sizeof (musicqitem_t));
  assert (musicqitem != NULL);

  musicqitem->dispidx = musicq->dispidx [musicqidx];
  ++(musicq->dispidx [musicqidx]);
  musicqitem->uniqueidx = musicq->uniqueidx [musicqidx];
  ++(musicq->uniqueidx [musicqidx]);
  musicqitem->song = song;
  musicqitem->playlistName = NULL;
  if (plname != NULL) {
    musicqitem->playlistName = strdup (plname);
  }
  musicqitem->announce = NULL;
  musicqitem->flags = MUSICQ_FLAG_NONE;
  dur = songGetNum (song, TAG_DURATION);
  musicq->duration [musicqidx] += dur;
  queuePush (musicq->q [musicqidx], musicqitem);
  logProcEnd (LOG_PROC, "musicqPush", "");
}

void
musicqPushHeadEmpty (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t      *musicqitem;

  logProcBegin (LOG_PROC, "musicqPushHeadEmpty");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqPushHeadEmpty", "bad-ptr");
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
  logProcEnd (LOG_PROC, "musicqPushHeadEmpty", "");
}

void
musicqMove (musicq_t *musicq, musicqidx_t musicqidx,
    ssize_t fromidx, ssize_t toidx)
{
  int   olddispidx;


  logProcBegin (LOG_PROC, "musicqMove");
  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueMove (musicq->q [musicqidx], fromidx, toidx);
  musicqRenumber (musicq, musicqidx, olddispidx);
  logProcEnd (LOG_PROC, "musicqMove", "");
}

int
musicqInsert (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx, song_t *song)
{
  int           olddispidx;
  musicqitem_t  *musicqitem;
  ssize_t       dur;

  logProcBegin (LOG_PROC, "musicqInsert");
  if (musicq == NULL || musicq->q [musicqidx] == NULL || song == NULL) {
    logProcEnd (LOG_PROC, "musicqInsert", "bad-ptr");
    return -1;
  }
  if (idx < 1) {
    logProcEnd (LOG_PROC, "musicqInsert", "bad-idx");
    return -1;
  }
  if (idx >= queueGetCount (musicq->q [musicqidx])) {
    musicqPush (musicq, musicqidx, song, NULL);
    logProcEnd (LOG_PROC, "musicqInsert", "idx>q-count; push");
    return (queueGetCount (musicq->q [musicqidx]) - 1);
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
  dur = songGetNum (song, TAG_DURATION);
  musicq->duration [musicqidx] += dur;

  queueInsert (musicq->q [musicqidx], idx, musicqitem);
  musicqRenumber (musicq, musicqidx, olddispidx);
  logProcEnd (LOG_PROC, "musicqInsert", "");
  return (idx - 1);
}

song_t *
musicqGetCurrent (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t      *musicqitem;

  logProcBegin (LOG_PROC, "musicqGetCurrent");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqGetCurrent", "bad-ptr");
    return NULL;
  }

  musicqitem = queueGetCurrent (musicq->q [musicqidx]);
  if (musicqitem == NULL) {
    logProcEnd (LOG_PROC, "musicqGetCurrent", "no-item");
    return NULL;
  }
  if ((musicqitem->flags & MUSICQ_FLAG_EMPTY) == MUSICQ_FLAG_EMPTY) {
    logProcEnd (LOG_PROC, "musicqGetCurrent", "empty-item");
    return NULL;
  }
  logProcEnd (LOG_PROC, "musicqGetCurrent", "");
  return musicqitem->song;
}

/* gets by the real idx, not the display index */
song_t *
musicqGetByIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey)
{
  musicqitem_t      *musicqitem;

  logProcBegin (LOG_PROC, "musicqGetByIdx");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqGetByIdx", "bad-ptr");
    return NULL;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd (LOG_PROC, "musicqGetByIdx", "");
    return musicqitem->song;
  }
  logProcEnd (LOG_PROC, "musicqGetByIdx", "no-item");
  return NULL;
}

musicqflag_t
musicqGetFlags (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey)
{
  musicqitem_t      *musicqitem;

  logProcBegin (LOG_PROC, "musicqGetFlags");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqGetFlags", "bad-ptr");
    return MUSICQ_FLAG_NONE;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd (LOG_PROC, "musicqGetFlags", "");
    return musicqitem->flags;
  }
  logProcEnd (LOG_PROC, "musicqGetFlags", "no-item");
  return MUSICQ_FLAG_NONE;
}

int
musicqGetDispIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey)
{
  musicqitem_t      *musicqitem;

  logProcBegin (LOG_PROC, "musicqGetDispIdx");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqGetDispIdx", "bad-ptr");
    return MUSICQ_FLAG_NONE;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd (LOG_PROC, "musicqGetDispIdx", "");
    return musicqitem->dispidx;
  }
  logProcEnd (LOG_PROC, "musicqGetDispIdx", "no-item");
  return -1;
}

int
musicqGetUniqueIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey)
{
  musicqitem_t      *musicqitem;

  logProcBegin (LOG_PROC, "musicqGetUniqueIdx");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqGetUniqueIdx", "bad-ptr");
    return MUSICQ_FLAG_NONE;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd (LOG_PROC, "musicqGetUniqueIdx", "");
    return musicqitem->uniqueidx;
  }
  logProcEnd (LOG_PROC, "musicqGetUniqueIdx", "no-item");
  return -1;
}

void
musicqSetFlag (musicq_t *musicq, musicqidx_t musicqidx,
    ssize_t qkey, musicqflag_t flags)
{
  musicqitem_t      *musicqitem;

  logProcBegin (LOG_PROC, "musicqSetFlag");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqSetFlag", "bad-ptr");
    return;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd (LOG_PROC, "musicqSetFlag", "");
    musicqitem->flags |= flags;
  }
  logProcEnd (LOG_PROC, "musicqSetFlag", "no-item");
  return;
}

void
musicqClearFlag (musicq_t *musicq, musicqidx_t musicqidx,
    ssize_t qkey, musicqflag_t flags)
{
  musicqitem_t      *musicqitem;

  logProcBegin (LOG_PROC, "musicqClearFlag");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqClearFlag", "bad-ptr");
    return;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd (LOG_PROC, "musicqClearFlag", "");
    musicqitem->flags &= ~flags;
  }
  logProcEnd (LOG_PROC, "musicqClearFlag", "no-item");
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
  musicqitem_t  *musicqitem;
  ssize_t       dur;
  song_t        *song;

  logProcBegin (LOG_PROC, "musicqPop");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqPop", "bad-ptr");
    return;
  }

  musicqitem = queuePop (musicq->q [musicqidx]);
  if (musicqitem == NULL) {
    return;
  }
  if ((musicqitem->flags & MUSICQ_FLAG_EMPTY) != MUSICQ_FLAG_EMPTY) {
    song = musicqitem->song;
    dur = songGetNum (song, TAG_DURATION);
    musicq->duration [musicqidx] -= dur;
  }
  musicqQueueItemFree (musicqitem);
  logProcEnd (LOG_PROC, "musicqPop", "");
}

/* does not clear the initial entry -- that's the song that is playing */
void
musicqClear (musicq_t *musicq, musicqidx_t musicqidx, ssize_t startIdx)
{
  int           olddispidx;
  ssize_t       iteridx;
  musicqitem_t  *musicqitem;
  song_t        *song;
  ssize_t       dur;

  logProcBegin (LOG_PROC, "musicqClear");
  if (startIdx < 1 || startIdx >= queueGetCount (musicq->q [musicqidx])) {
    logProcEnd (LOG_PROC, "musicqClear", "bad-idx");
    return;
  }

  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueClear (musicq->q [musicqidx], startIdx);

  queueStartIterator (musicq->q [musicqidx], &iteridx);
  musicq->duration [musicqidx] = 0;
  while ((musicqitem = queueIterateData (musicq->q [musicqidx], &iteridx)) != NULL) {
    song = musicqitem->song;
    dur = songGetNum (song, TAG_DURATION);
    musicq->duration [musicqidx] += dur;
  }

  musicq->dispidx [musicqidx] = olddispidx + queueGetCount (musicq->q [musicqidx]);
  logProcEnd (LOG_PROC, "musicqClear", "");
}

void
musicqRemove (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx)
{
  int           olddispidx;
  musicqitem_t  *musicqitem;
  song_t        *song;
  ssize_t       dur;


  logProcBegin (LOG_PROC, "musicqRemove");
  if (idx < 1 || idx >= queueGetCount (musicq->q [musicqidx])) {
    logProcEnd (LOG_PROC, "musicqRemove", "bad-idx");
    return;
  }


  musicqitem = queueGetByIdx (musicq->q [musicqidx], idx);
  song = musicqitem->song;
  dur = songGetNum (song, TAG_DURATION);
  musicq->duration [musicqidx] -= dur;

  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueRemoveByIdx (musicq->q [musicqidx], idx);

  musicqRenumber (musicq, musicqidx, olddispidx);
  --musicq->dispidx [musicqidx];

  musicqQueueItemFree (musicqitem);

  logProcEnd (LOG_PROC, "musicqRemove", "");
}

ssize_t
musicqGetLen (musicq_t *musicq, musicqidx_t musicqidx)
{
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return 0;
  }

  return queueGetCount (musicq->q [musicqidx]);
}


ssize_t
musicqGetDuration (musicq_t *musicq, musicqidx_t musicqidx)
{
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return 0;
  }

  return musicq->duration [musicqidx];
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
    data = songGetStr (song, tagidx);
  }
  return data;
}

char *
musicqGetDance (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx)
{
  musicqitem_t  *musicqitem = NULL;
  song_t        *song = NULL;
  listidx_t     danceIdx;
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
    danceIdx = songGetNum (song, TAG_DANCE);
    dances = bdjvarsdfGet (BDJVDF_DANCES);
    danceStr = danceGetStr (dances, danceIdx, DANCE_DANCE);
  }
  return danceStr;
}

musicqidx_t
musicqNextQueue (musicqidx_t musicqidx)
{
  ++musicqidx;
  if (musicqidx >= MUSICQ_MAX) {
    musicqidx = MUSICQ_A;
  }

  return musicqidx;
}

/* internal routines */

static void
musicqQueueItemFree (void *titem)
{
  musicqitem_t        *musicqitem = titem;

  logProcBegin (LOG_PROC, "musicqQueueItemFree");
  if (musicqitem != NULL) {
    if (musicqitem->playlistName != NULL) {
      free (musicqitem->playlistName);
    }
    if (musicqitem->announce != NULL) {
      free (musicqitem->announce);
    }
    free (musicqitem);
  }
  logProcEnd (LOG_PROC, "musicqQueueItemFree", "");
}

static int
musicqRenumberStart (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t  *musicqitem;
  int           dispidx = -1;

  logProcBegin (LOG_PROC, "musicqRenumberStart");
  musicqitem = queueGetByIdx (musicq->q [musicqidx], 0);
  if (musicqitem != NULL) {
    dispidx = musicqitem->dispidx;
  }
  logProcEnd (LOG_PROC, "musicqRenumberStart", "");
  return dispidx;
}

static void
musicqRenumber (musicq_t *musicq, musicqidx_t musicqidx, int olddispidx)
{
  int           dispidx = olddispidx;
  ssize_t       iteridx;
  musicqitem_t  *musicqitem;

  logProcBegin (LOG_PROC, "musicqRenumber");
  queueStartIterator (musicq->q [musicqidx], &iteridx);
  while ((musicqitem = queueIterateData (musicq->q [musicqidx], &iteridx)) != NULL) {
    musicqitem->dispidx = dispidx;
    ++dispidx;
  }
  logProcEnd (LOG_PROC, "musicqRenumber", "");
}

