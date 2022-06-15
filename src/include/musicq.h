#ifndef INC_MUSICQ_H
#define INC_MUSICQ_H

#include "musicdb.h"
#include "queue.h"
#include "tagdef.h"

typedef enum {
  MUSICQ_A,
  MUSICQ_B,
  /* if the number of music queues is changed, bdjopt.h and bdj4configui.c */
  /* must be updated */
  MUSICQ_MAX,
  MUSICQ_CURRENT,
} musicqidx_t;

typedef enum {
  MUSICQ_FLAG_NONE      = 0x0000,
  MUSICQ_FLAG_PREP      = 0x0001,
  MUSICQ_FLAG_ANNOUNCE  = 0x0002,
  MUSICQ_FLAG_PAUSE     = 0x0004,
  MUSICQ_FLAG_REQUEST   = 0x0008,
  MUSICQ_FLAG_EMPTY     = 0x0010,
} musicqflag_t;

typedef struct musicq musicq_t;

musicq_t *  musicqAlloc (musicdb_t *db);
void        musicqFree (musicq_t *musicq);
void        musicqSetDatabase (musicq_t *musicq, musicdb_t *db);
void        musicqPush (musicq_t *musicq, musicqidx_t idx, dbidx_t dbidx, char *plname);
void        musicqPushHeadEmpty (musicq_t *musicq, musicqidx_t idx);
void        musicqMove (musicq_t *musicq, musicqidx_t musicqidx,
                ssize_t fromidx, ssize_t toidx);
int         musicqInsert (musicq_t *musicq, musicqidx_t musicqidx,
                ssize_t idx, dbidx_t dbidx);
dbidx_t     musicqGetCurrent (musicq_t *musicq, musicqidx_t musicqidx);
musicqflag_t musicqGetFlags (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
int         musicqGetDispIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
int         musicqGetUniqueIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
void        musicqSetFlag (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey, musicqflag_t flags);
void        musicqClearFlag (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey, musicqflag_t flags);
char        *musicqGetAnnounce (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
void        musicqSetAnnounce (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey, char *annfname);
char        *musicqGetPlaylistName (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
dbidx_t     musicqGetByIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
void        musicqPop (musicq_t *musicq, musicqidx_t musicqidx);
void        musicqClear (musicq_t *musicq, musicqidx_t musicqidx, ssize_t startIdx);
void        musicqRemove (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx);
ssize_t     musicqGetLen (musicq_t *musicq, musicqidx_t musicqidx);
ssize_t     musicqGetDuration (musicq_t *musicq, musicqidx_t musicqidx);
char *      musicqGetDance (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx);
char *      musicqGetData (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx, tagdefkey_t tagidx);
musicqidx_t musicqNextQueue (musicqidx_t musicqidx);

#endif /* INC_MUSICQ_H */
