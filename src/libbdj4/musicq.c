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
#include "inline.h"
#include "log.h"
#include "musicdb.h"
#include "musicq.h"
#include "queue.h"
#include "song.h"
#include "tagdef.h"

typedef struct {
  int           dispidx;
  long          uniqueidx;
  dbidx_t       dbidx;
  int           playlistIdx;
  musicqflag_t  flags;
  char          *announce;
  long          dur;
} musicqitem_t;

typedef struct musicq {
  musicdb_t       *musicdb;
  queue_t         *q [MUSICQ_MAX];
  int             dispidx [MUSICQ_MAX];
  time_t          duration [MUSICQ_MAX];
} musicq_t;

static void musicqQueueItemFree (void *tqitem);
static int  musicqRenumberStart (musicq_t *musicq, musicqidx_t musicqidx);
static void musicqRenumber (musicq_t *musicq, musicqidx_t musicqidx, int olddispidx);

static long   guniqueidx = 0;

musicq_t *
musicqAlloc (musicdb_t *db)
{
  musicq_t  *musicq;

  logProcBegin (LOG_PROC, "musicqAlloc");
  musicq = malloc (sizeof (musicq_t));
  assert (musicq != NULL);
  musicq->musicdb = db;
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    char  tmp [40];

    snprintf (tmp, sizeof (tmp), "music-q-%d", i);
    musicq->q [i] = queueAlloc (tmp, musicqQueueItemFree);
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
    for (int i = 0; i < MUSICQ_MAX; ++i) {
      if (musicq->q [i] != NULL) {
        queueFree (musicq->q [i]);
      }
    }
    free (musicq);
  }
  logProcEnd (LOG_PROC, "musicqFree", "");
}

void
musicqSetDatabase (musicq_t *musicq, musicdb_t *db)
{
  if (musicq == NULL) {
    return;
  }

  musicq->musicdb = db;
}

void
musicqPush (musicq_t *musicq, musicqidx_t musicqidx, dbidx_t dbidx,
    int playlistIdx, long dur)
{
  musicqitem_t      *musicqitem;
  song_t            *song = NULL;

  logProcBegin (LOG_PROC, "musicqPush");
  if (musicq == NULL || musicq->q [musicqidx] == NULL || dbidx < 0) {
    logProcEnd (LOG_PROC, "musicqPush", "bad-ptr");
    return;
  }

  song = dbGetByIdx (musicq->musicdb, dbidx);
  if (song == NULL) {
    logProcEnd (LOG_PROC, "musicqPush", "song-not-found");
    return;
  }

  musicqitem = malloc (sizeof (musicqitem_t));
  assert (musicqitem != NULL);

  musicqitem->dispidx = musicq->dispidx [musicqidx];
  ++(musicq->dispidx [musicqidx]);
  musicqitem->uniqueidx = guniqueidx++;
  musicqitem->dbidx = dbidx;
  musicqitem->playlistIdx = playlistIdx;
  musicqitem->announce = NULL;
  musicqitem->flags = MUSICQ_FLAG_NONE;
  musicqitem->dur = dur;
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
  musicqitem->uniqueidx = guniqueidx++;
  musicqitem->dbidx = -1;
  musicqitem->playlistIdx = MUSICQ_PLAYLIST_EMPTY;
  musicqitem->announce = NULL;
  musicqitem->flags = MUSICQ_FLAG_EMPTY;
  musicqitem->dur = 0;
  queuePushHead (musicq->q [musicqidx], musicqitem);
  logProcEnd (LOG_PROC, "musicqPushHeadEmpty", "");
}

void
musicqMove (musicq_t *musicq, musicqidx_t musicqidx,
    qidx_t fromidx, qidx_t toidx)
{
  int   olddispidx;


  logProcBegin (LOG_PROC, "musicqMove");
  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueMove (musicq->q [musicqidx], fromidx, toidx);
  musicqRenumber (musicq, musicqidx, olddispidx);
  logProcEnd (LOG_PROC, "musicqMove", "");
}

int
musicqInsert (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx,
    dbidx_t dbidx, long dur)
{
  int           olddispidx;
  musicqitem_t  *musicqitem;
  song_t        *song;

  logProcBegin (LOG_PROC, "musicqInsert");
  if (musicq == NULL || musicq->q [musicqidx] == NULL || dbidx < 0) {
    logProcEnd (LOG_PROC, "musicqInsert", "bad-ptr");
    return -1;
  }
  if (idx < 1) {
    logProcEnd (LOG_PROC, "musicqInsert", "bad-idx");
    return -1;
  }

  song = dbGetByIdx (musicq->musicdb, dbidx);
  if (song == NULL) {
    logProcEnd (LOG_PROC, "musicqPush", "song-not-found");
    return -1;
  }

  if (idx >= queueGetCount (musicq->q [musicqidx])) {
    musicqPush (musicq, musicqidx, dbidx, MUSICQ_PLAYLIST_EMPTY, dur);
    logProcEnd (LOG_PROC, "musicqInsert", "idx>q-count; push");
    return (queueGetCount (musicq->q [musicqidx]) - 1);
  }

  olddispidx = musicqRenumberStart (musicq, musicqidx);

  musicqitem = malloc (sizeof (musicqitem_t));
  assert (musicqitem != NULL);
  musicqitem->dbidx = dbidx;
  musicqitem->playlistIdx = MUSICQ_PLAYLIST_EMPTY;
  musicqitem->announce = NULL;
  musicqitem->flags = MUSICQ_FLAG_REQUEST;
  musicqitem->uniqueidx = guniqueidx++;
  musicqitem->dur = dur;
  musicq->duration [musicqidx] += dur;

  queueInsert (musicq->q [musicqidx], idx, musicqitem);
  musicqRenumber (musicq, musicqidx, olddispidx);
  logProcEnd (LOG_PROC, "musicqInsert", "");
  return idx;
}

dbidx_t
musicqGetCurrent (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t      *musicqitem;

  logProcBegin (LOG_PROC, "musicqGetCurrent");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqGetCurrent", "bad-ptr");
    return -1;
  }

  musicqitem = queueGetFirst (musicq->q [musicqidx]);
  if (musicqitem == NULL) {
    logProcEnd (LOG_PROC, "musicqGetCurrent", "no-item");
    return -1;
  }
  if ((musicqitem->flags & MUSICQ_FLAG_EMPTY) == MUSICQ_FLAG_EMPTY) {
    logProcEnd (LOG_PROC, "musicqGetCurrent", "empty-item");
    return -1;
  }
  logProcEnd (LOG_PROC, "musicqGetCurrent", "");
  return musicqitem->dbidx;
}

/* gets by the real idx, not the display index */
dbidx_t
musicqGetByIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey)
{
  musicqitem_t      *musicqitem;

  logProcBegin (LOG_PROC, "musicqGetByIdx");
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqGetByIdx", "bad-ptr");
    return -1;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd (LOG_PROC, "musicqGetByIdx", "");
    return musicqitem->dbidx;
  }
  logProcEnd (LOG_PROC, "musicqGetByIdx", "no-item");
  return -1;
}

musicqflag_t
musicqGetFlags (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey)
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
musicqGetDispIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey)
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
musicqGetUniqueIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey)
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
    qidx_t qkey, musicqflag_t flags)
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
    qidx_t qkey, musicqflag_t flags)
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
musicqGetAnnounce (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey)
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
    qidx_t qkey, char *annfname)
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

int
musicqGetPlaylistIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return MUSICQ_PLAYLIST_EMPTY;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    return musicqitem->playlistIdx;
  }
  return MUSICQ_PLAYLIST_EMPTY;
}

void
musicqPop (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t  *musicqitem;

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
    musicq->duration [musicqidx] -= musicqitem->dur;
  }
  musicqQueueItemFree (musicqitem);
  logProcEnd (LOG_PROC, "musicqPop", "");
}

/* does not clear the initial entry -- that's the song that is playing */
void
musicqClear (musicq_t *musicq, musicqidx_t musicqidx, qidx_t startIdx)
{
  int           olddispidx;
  qidx_t        qiteridx;
  musicqitem_t  *musicqitem;

  logProcBegin (LOG_PROC, "musicqClear");
  if (musicq == NULL) {
    logProcEnd (LOG_PROC, "musicqClear", "bad-ptr");
    return;
  }
  if (musicq->q [musicqidx] == NULL) {
    logProcEnd (LOG_PROC, "musicqClear", "bad-ptr-b");
    return;
  }

  if (startIdx < 1 || startIdx >= queueGetCount (musicq->q [musicqidx])) {
    logProcEnd (LOG_PROC, "musicqClear", "bad-idx");
    return;
  }

  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueClear (musicq->q [musicqidx], startIdx);

  queueStartIterator (musicq->q [musicqidx], &qiteridx);
  /* just re-calculate the duration for the queue */
  musicq->duration [musicqidx] = 0;
  while ((musicqitem = queueIterateData (musicq->q [musicqidx], &qiteridx)) != NULL) {
    musicq->duration [musicqidx] += musicqitem->dur;
  }

  musicq->dispidx [musicqidx] = olddispidx + queueGetCount (musicq->q [musicqidx]);
  logProcEnd (LOG_PROC, "musicqClear", "");
}

void
musicqRemove (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx)
{
  int           olddispidx;
  musicqitem_t  *musicqitem;


  logProcBegin (LOG_PROC, "musicqRemove");
  if (idx < 1 || idx >= queueGetCount (musicq->q [musicqidx])) {
    logProcEnd (LOG_PROC, "musicqRemove", "bad-idx");
    return;
  }


  musicqitem = queueGetByIdx (musicq->q [musicqidx], idx);
  musicq->duration [musicqidx] -= musicqitem->dur;

  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueRemoveByIdx (musicq->q [musicqidx], idx);

  musicqRenumber (musicq, musicqidx, olddispidx);
  --musicq->dispidx [musicqidx];

  musicqQueueItemFree (musicqitem);

  logProcEnd (LOG_PROC, "musicqRemove", "");
}

int
musicqGetLen (musicq_t *musicq, musicqidx_t musicqidx)
{
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return 0;
  }

  return (int) queueGetCount (musicq->q [musicqidx]);
}


time_t
musicqGetDuration (musicq_t *musicq, musicqidx_t musicqidx)
{
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return 0;
  }

  return musicq->duration [musicqidx];
}


char *
musicqGetData (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx, tagdefkey_t tagidx)
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
    song = dbGetByIdx (musicq->musicdb, musicqitem->dbidx);
    data = songGetStr (song, tagidx);
  }
  return data;
}

char *
musicqGetDance (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx)
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
    song = dbGetByIdx (musicq->musicdb, musicqitem->dbidx);
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
  if ((int) musicqidx >= MUSICQ_PB_MAX) {
    musicqidx = MUSICQ_PB_A;
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
    dataFree (musicqitem->announce);
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
  qidx_t        qiteridx;
  musicqitem_t  *musicqitem;

  logProcBegin (LOG_PROC, "musicqRenumber");
  queueStartIterator (musicq->q [musicqidx], &qiteridx);
  while ((musicqitem = queueIterateData (musicq->q [musicqidx], &qiteridx)) != NULL) {
    musicqitem->dispidx = dispidx;
    ++dispidx;
  }
  logProcEnd (LOG_PROC, "musicqRenumber", "");
}

