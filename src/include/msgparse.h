/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_MSGPARSE_H
#define INC_MSGPARSE_H

#include "nlist.h"
#include "musicdb.h"
#include "musicq.h"

/* used for music queue update processing */
typedef struct {
  dbidx_t   dbidx;
  int       dispidx;
  long      uniqueidx;
  int       pauseind;
} mp_musicqupditem_t;

typedef struct {
  musicqidx_t     mqidx;
  long            tottime;
  dbidx_t         currdbidx;
  nlist_t         *dispList;
} mp_musicqupdate_t;

typedef struct {
  int             mqidx;
  int             loc;
} mp_songselect_t;

typedef struct {
  bool      repeat;
  bool      pauseatend;
  int       currentVolume;
  int       currentSpeed;
  int       baseVolume;
  uint64_t  playedtime;
  int64_t   duration;
} mp_playerstatus_t;

mp_musicqupdate_t *msgparseMusicQueueData (char * args);
void  msgparseMusicQueueDataFree (mp_musicqupdate_t *musicqupdate);
mp_songselect_t *msgparseSongSelect (char * args);
void msgparseSongSelectFree (mp_songselect_t *songselect);
mp_playerstatus_t *msgparsePlayerStatusData (char * args);
void msgparsePlayerStatusFree (mp_playerstatus_t *playerstatus);

void msgbuildQueuePlaylist (char *buff, size_t sz, int mqidx, const char *fn, int editflag);

#endif /* INC_MSGPARSE_H */
