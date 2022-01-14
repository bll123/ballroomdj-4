#ifndef INC_MUSICQ_H
#define INC_MUSICQ_H

#include "queue.h"
#include "song.h"

typedef enum {
  MUSICQ_A,
  MUSICQ_B,
  MUSICQ_MAX,
} musicqidx_t;

typedef enum {
  MUSICQ_FLAG_NONE  = 0x0000,
  MUSICQ_FLAG_PREP  = 0x0001,
} musicqflag_t;

typedef struct {
  song_t        *song;
  char          *playlistName;
  musicqflag_t  flags;
} musicqitem_t;

typedef struct {
  queue_t         *q [MUSICQ_MAX];
} musicq_t;

musicq_t *  musicqAlloc (void);
void        musicqFree (musicq_t *musicq);
void        musicqPush (musicq_t *musicq, musicqidx_t idx,
                song_t *song, char *plname);
song_t      *musicqGetCurrent (musicq_t *musicq, musicqidx_t musicqidx);
musicqflag_t musicqGetFlags (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
void        musicqSetFlags (musicq_t *musicq, musicqidx_t musicqidx,
                ssize_t qkey, musicqflag_t flags);
char        *musicqGetPlaylistName (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
song_t      *musicqGetByIdx (musicq_t *musicq, musicqidx_t musicqidx, ssize_t qkey);
void        musicqPop (musicq_t *musicq, musicqidx_t musicqidx);
ssize_t     musicqGetLen (musicq_t *musicq, musicqidx_t musicqidx);

#endif /* INC_MUSICQ_H */
