#ifndef INC_MUSICQ_H
#define INC_MUSICQ_H

#include "queue.h"
#include "song.h"

typedef enum {
  MUSICQ_A,
  MUSICQ_B,
  MUSICQ_MAX,
} musicqidx_t;

typedef struct {
  song_t      *song;
} musicqitem_t;

typedef struct {
  queue_t         *q [MUSICQ_MAX];
} musicq_t;

musicq_t *  musicqAlloc (void);
void        musicqFree (musicq_t *musicq);
void        musicqPush (musicq_t *musicq, musicqidx_t idx, song_t *song);
song_t      *musicqGetCurrent (musicq_t *musicq, musicqidx_t idx);
song_t      *musicqGetByIdx (musicq_t *musicq, musicqidx_t musicqidx, listidx_t idx);
void        musicqPop (musicq_t *musicq, musicqidx_t idx);
ssize_t     musicqGetLen (musicq_t *musicq, musicqidx_t idx);

#endif /* INC_MUSICQ_H */
