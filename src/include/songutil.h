#ifndef INC_SONGUTIL_H
#define INC_SONGUTIL_H

enum {
  SONG_ADJUST_NONE    = 0x0000,
  SONG_ADJUST_NORM    = 0x0001,
  SONG_ADJUST_TRIM    = 0x0002,
  SONG_ADJUST_SPEED   = 0x0004,
};

enum {
  SONG_ADJUST_STR_NORM = 'N',
  SONG_ADJUST_STR_TRIM = 'T',
  SONG_ADJUST_STR_SPEED = 'S',
};

char  *songFullFileName (char *sfname);
void  songConvAdjustFlags (datafileconv_t *conv);

#endif /* INC_SONGUTIL_H */
