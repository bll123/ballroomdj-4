/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "datafile.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* song adjustments will be removed at a later date */

enum {
  SONG_ADJUST_NONE    = 0,
  /* normalize didn't work, not currently in use, but keep it here */
  SONG_ADJUST_NORM    = (1 << 0),
  /* trim is no longer in use */
  SONG_ADJUST_TRIM    = (1 << 1),
  /* song-adjust-adjust adjusts the speed, song-start and song-end */
  SONG_ADJUST_ADJUST  = (1 << 2),
  /* for testing */
  SONG_ADJUST_TEST    = (1 << 3),
  /* repair reads in the audio file and re-writes it */
  /* not in use */
  SONG_ADJUST_REPAIR  = (1 << 4),
  /* special setting used for response from uiapplyadj */
  SONG_ADJUST_RESTORE = (1 << 5),
  /* song-adjust-invalid checks for anything other than the normal settings */
  SONG_ADJUST_INVALID =
      ~ (SONG_ADJUST_NORM | SONG_ADJUST_TRIM | SONG_ADJUST_ADJUST | SONG_ADJUST_TEST),
};

enum {
  SONG_ADJUST_STR_NORM = 'N',
  SONG_ADJUST_STR_TRIM = 'T',
  SONG_ADJUST_STR_ADJUST = 'S',
  SONG_ADJUST_STR_TEST = 'X',
};

void  songutilConvAdjustFlags (datafileconv_t *conv);
void  songutilConvSongType (datafileconv_t *conv);
ssize_t songutilAdjustPosReal (ssize_t pos, int speed);
ssize_t songutilNormalizePosition (ssize_t pos, int speed);
int   songutilAdjustBPM (int bpm, int speed);
int   songutilNormalizeBPM (int bpm, int speed);
void  songutilTitleFromWorkMovement (nlist_t *taglist);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

