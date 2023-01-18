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
  SONG_ADJUST_SPEED   = 0x0004,
  SONG_ADJUST_INVALID =
      ~ (SONG_ADJUST_NORM | SONG_ADJUST_TRIM | SONG_ADJUST_SPEED),
};

enum {
  SONG_ADJUST_STR_NORM = 'N',
  SONG_ADJUST_STR_TRIM = 'T',
  SONG_ADJUST_STR_SPEED = 'S',
};

char  *songFullFileName (const char *sfname);
void  songConvAdjustFlags (datafileconv_t *conv);
ssize_t songAdjustPosition (ssize_t pos, int speed);
ssize_t songNormalizePosition (ssize_t pos, int speed);

#endif /* INC_SONGUTIL_H */
