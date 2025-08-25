/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "datafile.h"
#include "ilist.h"
#include "nlist.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  SONG_NORM,
  SONG_UNADJUSTED_DURATION,
  SONG_LONG_DURATION,
};

enum {
  SONG_TYPE_NORM,
  SONG_TYPE_REMOTE,
  SONG_TYPE_PODCAST,
  SONG_TYPE_MAX,
};

typedef struct song song_t;

song_t *  songAlloc (void);
void      songFree (void *);
void      songFromTagList (song_t *song, slist_t *tagdata);
void      songParse (song_t *song, char *data, ilistidx_t didx);
const char *songGetStr (const song_t *, nlistidx_t);
listnum_t songGetNum (const song_t *, nlistidx_t);
double    songGetDouble (const song_t *, nlistidx_t);
slist_t * songGetList (const song_t *song, nlistidx_t idx);
void      songGetClassicalWork (const song_t *song, char *work, size_t sz);
void      songSetNum (song_t *, nlistidx_t, listnum_t val);
void      songSetDouble (song_t *, nlistidx_t, double val);
void      songSetStr (song_t *song, nlistidx_t tagidx, const char *str);
void      songSetList (song_t *song, nlistidx_t tagidx, const char *str);
void      songChangeFavorite (song_t *song);
bool      songAudioSourceExists (song_t *song);
char *    songDisplayString (song_t *song, int tagidx, int flag);
slist_t * songTagList (song_t *song);
char *    songCreateSaveData (song_t *song);
bool      songIsChanged (song_t *song);
void      songSetChanged (song_t *song);
bool      songHasSonglistChange (song_t *song);
void      songClearChanged (song_t *song);
//void      songDump (song_t *song);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

