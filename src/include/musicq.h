/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_MUSICQ_H
#define INC_MUSICQ_H

#include <time.h>

#include "bdj4.h"
#include "musicdb.h"
#include "queue.h"
#include "tagdef.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  /* must match the number of music queues defined in bdj4.h (BDJ4_QUEUE_MAX) */
  MUSICQ_PB_A,
  MUSICQ_PB_B,
  MUSICQ_PB_C,
  MUSICQ_PB_D,
  MUSICQ_PB_E,
  /* keep the history music queue after the last player music queue */
  /* this makes processing easier */
  MUSICQ_HISTORY,
  MUSICQ_SL,                        // for the song list editor
  MUSICQ_MNG_PB,                    // hidden q used by manageui for playback
  MUSICQ_MAX,
} musicqidx_t;

enum {
  MUSICQ_PB_MAX = BDJ4_QUEUE_MAX,   // music queues not including history
  MUSICQ_DISP_MAX = MUSICQ_SL,      // music queues including history
};

typedef enum {
  MUSICQ_FLAG_NONE      = 0,
  MUSICQ_FLAG_PREP      = (1 << 0),
  MUSICQ_FLAG_ANNOUNCE  = (1 << 1),
  MUSICQ_FLAG_PAUSE     = (1 << 2),
  MUSICQ_FLAG_REQUEST   = (1 << 3),
  MUSICQ_FLAG_EMPTY     = (1 << 4),
} musicqflag_t;

enum {
  MUSICQ_PLAYLIST_EMPTY = -1,
};

typedef struct musicq musicq_t;

musicq_t *  musicqAlloc (musicdb_t *db);
void        musicqFree (musicq_t *musicq);
void        musicqSetDatabase (musicq_t *musicq, musicdb_t *db);
void        musicqPush (musicq_t *musicq, musicqidx_t idx, dbidx_t dbidx,
                int playlistIdx, long dur);
void        musicqPushHeadEmpty (musicq_t *musicq, musicqidx_t idx);
void        musicqMove (musicq_t *musicq, musicqidx_t musicqidx,
                qidx_t fromidx, qidx_t toidx);
int         musicqInsert (musicq_t *musicq, musicqidx_t musicqidx,
                qidx_t idx, dbidx_t dbidx, long dur);
dbidx_t     musicqGetCurrent (musicq_t *musicq, musicqidx_t musicqidx);
musicqflag_t musicqGetFlags (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey);
int         musicqGetDispIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey);
long        musicqGetUniqueIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey);
void        musicqSetFlag (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey, musicqflag_t flags);
void        musicqClearFlag (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey, musicqflag_t flags);
char        *musicqGetAnnounce (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey);
void        musicqSetAnnounce (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey, const char *annfname);
int         musicqGetPlaylistIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey);
dbidx_t     musicqGetByIdx (musicq_t *musicq, musicqidx_t musicqidx, qidx_t qkey);
void        musicqPop (musicq_t *musicq, musicqidx_t musicqidx);
void        musicqClear (musicq_t *musicq, musicqidx_t musicqidx, qidx_t startIdx);
void        musicqRemove (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx);
void        musicqSwap (musicq_t *musicq, musicqidx_t musicqidx, qidx_t fromidx, qidx_t toidx);
int         musicqGetLen (musicq_t *musicq, musicqidx_t musicqidx);
time_t      musicqGetDuration (musicq_t *musicq, musicqidx_t musicqidx);
const char *musicqGetDance (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx);
const char *musicqGetData (musicq_t *musicq, musicqidx_t musicqidx, qidx_t idx, tagdefkey_t tagidx);
musicqidx_t musicqNextQueue (musicqidx_t musicqidx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_MUSICQ_H */
