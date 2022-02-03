#ifndef INC_MUSICQ_H
#define INC_MUSICQ_H

#include "queue.h"
#include "song.h"
#include "tagdef.h"

typedef enum {
  MUSICQ_A,
  MUSICQ_B,
  MUSICQ_MAX,
} musicqidx_t;

typedef enum {
  MUSICQ_FLAG_NONE      = 0x0000,
  MUSICQ_FLAG_PREP      = 0x0001,
  MUSICQ_FLAG_ANNOUNCE  = 0x0002,
  MUSICQ_FLAG_PAUSE     = 0x0004,
  MUSICQ_FLAG_REQUEST   = 0x0008,
  MUSICQ_FLAG_EMPTY     = 0x0010,
} musicqflag_t;

typedef struct {
  int           dispidx;
  int           uniqueidx;
  song_t        *song;
  char          *playlistName;
  musicqflag_t  flags;
  char          *announce;
} musicqitem_t;

typedef struct {
  queue_t         *q [MUSICQ_MAX];
  int             dispidx [MUSICQ_MAX];
  int             uniqueidx [MUSICQ_MAX];
} musicq_t;

musicq_t *  musicqAlloc (void);
void        musicqFree (musicq_t *musicq);
void        musicqPush (musicq_t *musicq, musicqidx_t idx,
                song_t *song, char *plname);
void        musicqPushHeadEmpty (musicq_t *musicq, musicqidx_t idx);
void        musicqMove (musicq_t *musicq, musicqidx_t musicqidx,
                ssize_t fromidx, ssize_t toidx);
void        musicqInsert (musicq_t *musicq, musicqidx_t musicqidx,
                ssize_t idx, song_t *song);
song_t      *musicqGetCurrent (musicq_t *musicq, musicqidx_t musicqidx);
musicqflag_t musicqGetFlags (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
int         musicqGetDispIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
int         musicqGetUniqueIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
void        musicqSetFlag (musicq_t *musicq, musicqidx_t musicqidx,
                ssize_t qkey, musicqflag_t flags);
void        musicqClearFlag (musicq_t *musicq, musicqidx_t musicqidx,
                ssize_t qkey, musicqflag_t flags);
char        *musicqGetAnnounce (musicq_t *musicq, musicqidx_t musicqidx,
                ssize_t qkey);
void        musicqSetAnnounce (musicq_t *musicq, musicqidx_t musicqidx,
                ssize_t qkey, char *annfname);
char        *musicqGetPlaylistName (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
song_t      *musicqGetByIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
void        musicqPop (musicq_t *musicq, musicqidx_t musicqidx);
void        musicqClear (musicq_t *musicq, musicqidx_t musicqidx, ssize_t startIdx);
void        musicqRemove (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx);
ssize_t     musicqGetLen (musicq_t *musicq, musicqidx_t musicqidx);
char *      musicqGetDance (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx);
char *      musicqGetData (musicq_t *musicq, musicqidx_t musicqidx, ssize_t idx, tagdefkey_t tagidx);

#endif /* INC_MUSICQ_H */
