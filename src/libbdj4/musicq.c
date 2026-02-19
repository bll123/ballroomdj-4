/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "bdjvarsdf.h"
#include "dance.h"
#include "log.h"
#include "mdebug.h"
#include "nodiscard.h"
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

BDJ_NODISCARD
musicq_t *
musicqAlloc (musicdb_t *db)
{
  musicq_t  *musicq;

  logProcBegin ();

  musicq = mdmalloc (sizeof (musicq_t));
  musicq->musicdb = db;
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    char  tmp [40];

    snprintf (tmp, sizeof (tmp), "music-q-%d", i);
    musicq->q [i] = queueAlloc (tmp, musicqQueueItemFree);
    musicq->dispidx [i] = 1;
    musicq->duration [i] = 0;
  }
  logProcEnd ("");
  return musicq;
}

void
musicqFree (musicq_t *musicq)
{
  logProcBegin ();
  if (musicq != NULL) {
    for (int i = 0; i < MUSICQ_MAX; ++i) {
      if (musicq->q [i] != NULL) {
        queueFree (musicq->q [i]);
      }
    }
    mdfree (musicq);
  }
  logProcEnd ("");
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
    int playlistIdx, int64_t dur)
{
  musicqitem_t      *musicqitem;
  song_t            *song = NULL;

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL || dbidx < 0) {
    logProcEnd ("bad-ptr-or-bad-dbidx");
    return;
  }

  song = dbGetByIdx (musicq->musicdb, dbidx);
  if (song == NULL) {
    logProcEnd ("song-not-found");
    return;
  }

  musicqitem = mdmalloc (sizeof (musicqitem_t));

  musicqitem->dispidx = musicq->dispidx [musicqidx];
  musicqitem->dbidx = dbidx;
  musicqitem->playlistIdx = playlistIdx;
  musicqitem->announce = NULL;
  musicqitem->flags = MUSICQ_FLAG_NONE;
  ++(musicq->dispidx [musicqidx]);
  musicqitem->uniqueidx = guniqueidx++;
  musicqitem->dur = dur;
  musicq->duration [musicqidx] += dur;
  queuePush (musicq->q [musicqidx], musicqitem);
  logProcEnd ("");
}

void
musicqPushHeadEmpty (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t      *musicqitem;

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd ("bad-ptr");
    return;
  }

  musicqitem = mdmalloc (sizeof (musicqitem_t));
  musicqitem->dbidx = DBIDX_NONE;
  musicqitem->playlistIdx = MUSICQ_PLAYLIST_EMPTY;
  musicqitem->announce = NULL;
  musicqitem->flags = MUSICQ_FLAG_EMPTY;
  musicqitem->dispidx = 0;
  musicqitem->uniqueidx = guniqueidx++;
  musicqitem->dur = 0;
  queuePushHead (musicq->q [musicqidx], musicqitem);
  logProcEnd ("");
}

void
musicqMove (musicq_t *musicq, musicqidx_t musicqidx,
    qidx_t fromidx, qidx_t toidx)
{
  int   olddispidx;


  logProcBegin ();
  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueMove (musicq->q [musicqidx], fromidx, toidx);
  musicqRenumber (musicq, musicqidx, olddispidx);
  logProcEnd ("");
}

int
musicqInsert (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx,
    dbidx_t dbidx, int64_t dur)
{
  int           olddispidx;
  musicqitem_t  *musicqitem;
  song_t        *song;

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL || dbidx < 0) {
    logProcEnd ("bad-ptr");
    return -1;
  }
  if (idx < 1) {
    logProcEnd ("bad-idx");
    return -1;
  }

  song = dbGetByIdx (musicq->musicdb, dbidx);
  if (song == NULL) {
    logProcEnd ("song-not-found");
    return -1;
  }

  if (idx >= queueGetCount (musicq->q [musicqidx])) {
    musicqPush (musicq, musicqidx, dbidx, MUSICQ_PLAYLIST_EMPTY, dur);
    logProcEnd ("idx>q-count; push");
    return (queueGetCount (musicq->q [musicqidx]) - 1);
  }

  olddispidx = musicqRenumberStart (musicq, musicqidx);

  musicqitem = mdmalloc (sizeof (musicqitem_t));
  musicqitem->dbidx = dbidx;
  musicqitem->playlistIdx = MUSICQ_PLAYLIST_EMPTY;
  musicqitem->announce = NULL;
  musicqitem->flags = MUSICQ_FLAG_REQUEST;
  musicqitem->uniqueidx = guniqueidx++;
  musicqitem->dispidx = 0;
  musicqitem->dur = dur;
  musicq->duration [musicqidx] += dur;

  queueInsert (musicq->q [musicqidx], idx, musicqitem);
  musicqRenumber (musicq, musicqidx, olddispidx);
  logProcEnd ("");
  return idx;
}

dbidx_t
musicqGetCurrent (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t      *musicqitem;

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd ("bad-ptr");
    return DBIDX_NONE;
  }

  musicqitem = queueGetFirst (musicq->q [musicqidx]);
  if (musicqitem == NULL) {
    logProcEnd ("no-item");
    return DBIDX_NONE;
  }
  if ((musicqitem->flags & MUSICQ_FLAG_EMPTY) == MUSICQ_FLAG_EMPTY) {
    logProcEnd ("empty-item");
    return DBIDX_NONE;
  }
  logProcEnd ("");
  return musicqitem->dbidx;
}

/* gets by the real idx, not the display index */
dbidx_t
musicqGetByIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey)
{
  musicqitem_t      *musicqitem;

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd ("bad-ptr");
    return DBIDX_NONE;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd ("");
    return musicqitem->dbidx;
  }
  logProcEnd ("no-item");
  return DBIDX_NONE;
}

musicqflag_t
musicqGetFlags (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey)
{
  musicqitem_t      *musicqitem;

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd ("bad-ptr");
    return MUSICQ_FLAG_NONE;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd ("");
    return musicqitem->flags;
  }
  logProcEnd ("no-item");
  return MUSICQ_FLAG_NONE;
}

int
musicqGetDispIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey)
{
  musicqitem_t      *musicqitem;

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd ("bad-ptr");
    return MUSICQ_FLAG_NONE;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd ("");
    return musicqitem->dispidx;
  }
  logProcEnd ("no-item");
  return -1;
}

long
musicqGetUniqueIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey)
{
  musicqitem_t      *musicqitem;

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd ("bad-ptr");
    return MUSICQ_FLAG_NONE;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd ("");
    return musicqitem->uniqueidx;
  }
  logProcEnd ("no-item");
  return -1;
}

void
musicqSetFlag (musicq_t *musicq, musicqidx_t musicqidx,
    qidx_t qkey, musicqflag_t flags)
{
  musicqitem_t      *musicqitem;

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd ("bad-ptr");
    return;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd ("");
    musicqitem->flags |= flags;
  }
  logProcEnd ("no-item");
  return;
}

void
musicqClearFlag (musicq_t *musicq, musicqidx_t musicqidx,
    qidx_t qkey, musicqflag_t flags)
{
  musicqitem_t      *musicqitem;

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd ("bad-ptr");
    return;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    logProcEnd ("");
    musicqitem->flags &= ~flags;
  }
  logProcEnd ("no-item");
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
    qidx_t qkey, const char *annfname)
{
  musicqitem_t      *musicqitem;

  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    return;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], qkey);
  if (musicqitem != NULL) {
    musicqitem->announce = mdstrdup (annfname);
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

  logProcBegin ();
  if (musicq == NULL || musicq->q [musicqidx] == NULL) {
    logProcEnd ("bad-ptr");
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

  /* this handles the case where the player's queue is now empty */
  /* otherwise the display index resumes at the last known display index */
  if (queueGetCount (musicq->q [musicqidx]) == 0) {
    musicq->dispidx [musicqidx] = 1;
  }
  logProcEnd ("");
}

/* does not clear the initial entry -- that's the song that is playing */
void
musicqClear (musicq_t *musicq, musicqidx_t musicqidx, qidx_t startIdx)
{
  int           olddispidx;
  qidx_t        qiteridx;
  musicqitem_t  *musicqitem;

  logProcBegin ();
  if (musicq == NULL) {
    logProcEnd ("bad-ptr");
    return;
  }
  if (musicq->q [musicqidx] == NULL) {
    logProcEnd ("bad-ptr-b");
    return;
  }

  if (startIdx < 1 || startIdx >= queueGetCount (musicq->q [musicqidx])) {
    logProcEnd ("bad-idx");
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
  logProcEnd ("");
}

void
musicqRemove (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx)
{
  int           olddispidx;
  musicqitem_t  *musicqitem;


  logProcBegin ();
  if (idx < 1 || idx >= queueGetCount (musicq->q [musicqidx])) {
    logProcEnd ("bad-idx");
    return;
  }

  musicqitem = queueGetByIdx (musicq->q [musicqidx], idx);
  musicq->duration [musicqidx] -= musicqitem->dur;

  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueRemoveByIdx (musicq->q [musicqidx], idx);
  musicqRenumber (musicq, musicqidx, olddispidx);
  musicqQueueItemFree (musicqitem);

  logProcEnd ("");
}

void
musicqSwap (musicq_t *musicq, musicqidx_t musicqidx, qidx_t fromidx, qidx_t toidx)
{
  int           olddispidx;


  logProcBegin ();
  if (fromidx < 0 || fromidx >= queueGetCount (musicq->q [musicqidx])) {
    logProcEnd ("bad-idx-from");
    return;
  }
  if (toidx < 0 || toidx >= queueGetCount (musicq->q [musicqidx])) {
    logProcEnd ("bad-idx-to");
    return;
  }

  olddispidx = musicqRenumberStart (musicq, musicqidx);
  queueMove (musicq->q [musicqidx], fromidx, toidx);
  musicqRenumber (musicq, musicqidx, olddispidx);

  logProcEnd ("");
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


const char *
musicqGetData (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx, tagdefkey_t tagidx)
{
  musicqitem_t  *musicqitem;
  song_t        *song;
  const char    *data = NULL;

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

const char *
musicqGetDance (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx)
{
  musicqitem_t  *musicqitem = NULL;
  song_t        *song = NULL;
  listidx_t     danceIdx;
  const char    *danceStr = NULL;
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

  logProcBegin ();
  if (musicqitem != NULL) {
    dataFree (musicqitem->announce);
    mdfree (musicqitem);
  }
  logProcEnd ("");
}

static int
musicqRenumberStart (musicq_t *musicq, musicqidx_t musicqidx)
{
  musicqitem_t  *musicqitem;
  int           dispidx = -1;

  logProcBegin ();
  musicqitem = queueGetByIdx (musicq->q [musicqidx], 0);
  if (musicqitem != NULL) {
    dispidx = musicqitem->dispidx;
  }
  logProcEnd ("");
  return dispidx;
}

static void
musicqRenumber (musicq_t *musicq, musicqidx_t musicqidx, int olddispidx)
{
  int           dispidx = olddispidx;
  qidx_t        qiteridx;
  musicqitem_t  *musicqitem;

  logProcBegin ();
  queueStartIterator (musicq->q [musicqidx], &qiteridx);
  while ((musicqitem = queueIterateData (musicq->q [musicqidx], &qiteridx)) != NULL) {
    musicqitem->dispidx = dispidx;
    ++dispidx;
  }
  musicq->dispidx [musicqidx] = dispidx;
  logProcEnd ("");
}

