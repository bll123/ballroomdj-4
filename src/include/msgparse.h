/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdint.h>

#include "nodiscard.h"
#include "nlist.h"
#include "musicdb.h"
#include "musicq.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* used for music queue update processing */
typedef struct {
  dbidx_t   dbidx;
  int       dispidx;
  int32_t   uniqueidx;
  int       pauseind;
} mp_musicqupditem_t;

typedef struct {
  musicqidx_t     mqidx;
  int32_t         tottime;
  dbidx_t         currdbidx;
  nlist_t         *dispList;
} mp_musicqupdate_t;

typedef struct {
  int             mqidx;
  int32_t         loc;
} mp_songselect_t;

typedef struct {
  int       currentVolume;
  int       currentSpeed;
  int       baseVolume;
  uint32_t  playedtime;
  int32_t   duration;
  bool      repeat : 1;
  bool      pauseatend : 1;
} mp_playerstatus_t;

typedef struct {
  int             playerState;
  bool            newsong : 1;
} mp_playerstate_t;

typedef struct {
  dbidx_t   dbidx;
  int32_t   uniqueidx;
  char      *imguri;
} mp_musicqstatus_t;

NODISCARD mp_musicqupdate_t *msgparseMusicQueueData (char * data);
void  msgparseMusicQueueDataFree (mp_musicqupdate_t *musicqupdate);

NODISCARD mp_songselect_t *msgparseSongSelect (char * data);
void msgparseSongSelectFree (mp_songselect_t *songselect);

void msgbuildPlayerStatus (char *buff, size_t sz, bool repeat, bool pauseatend, int currvol, int currspeed, int basevol, uint32_t tm, int32_t dur);
NODISCARD mp_playerstatus_t *msgparsePlayerStatusData (char * data);
void msgparsePlayerStatusFree (mp_playerstatus_t *playerstatus);

void msgbuildPlayerState (char *buff, size_t sz, int playerState, bool newsong);
NODISCARD mp_playerstate_t *msgparsePlayerStateData (char * data);
void msgparsePlayerStateFree (mp_playerstate_t *playerstate);

void msgbuildQueuePlaylist (char *buff, size_t sz, int mqidx, const char *fn, int editflag);

void msgbuildMusicQStatus (char *buff, size_t sz, dbidx_t dbidx, int32_t uniqueidx, const char *imguri);
void msgparseMusicQStatus (mp_musicqstatus_t *mqstatus, char *data);

void msgparseDBEntryUpdate (char *data, dbidx_t *dbidx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

