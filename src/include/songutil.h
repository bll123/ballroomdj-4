/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SONGUTIL_H
#define INC_SONGUTIL_H

#include "datafile.h"

enum {
  SONG_ADJUST_NONE    = 0x0000,
  SONG_ADJUST_NORM    = 0x0001,
  SONG_ADJUST_TRIM    = 0x0002,
  /* song-adjust-adjust adjusts the speed, song-start and song-end */
  SONG_ADJUST_ADJUST  = 0x0004,
  /* repair reads in the audio file and re-writes it */
  SONG_ADJUST_REPAIR  = 0x0008,
  /* special setting used for response from uiapplyadj */
  SONG_ADJUST_RESTORE = 0x0010,
  /* song-adjust-invalid checks for anything other than the normal settings */
  SONG_ADJUST_INVALID =
      ~ (SONG_ADJUST_NORM | SONG_ADJUST_TRIM | SONG_ADJUST_ADJUST),
};

enum {
  SONG_ADJUST_STR_NORM = 'N',
  SONG_ADJUST_STR_TRIM = 'T',
  SONG_ADJUST_STR_ADJUST = 'S',
};

char  *songutilFullFileName (const char *sfname);
bool  songutilHasOriginal (const char *sfname);
void  songutilConvAdjustFlags (datafileconv_t *conv);
ssize_t songutilAdjustPosReal (ssize_t pos, int speed);
ssize_t songutilNormalizePosition (ssize_t pos, int speed);
int   songutilAdjustBPM (int bpm, int speed);
int   songutilNormalizeBPM (int bpm, int speed);
const char * songutilGetRelativePath (const char *fn);

#endif /* INC_SONGUTIL_H */
