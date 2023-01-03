/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ITUNES_H
#define INC_ITUNES_H

enum {
  ITUNES_STARS_10,
  ITUNES_STARS_20,
  ITUNES_STARS_30,
  ITUNES_STARS_40,
  ITUNES_STARS_50,
  ITUNES_STARS_60,
  ITUNES_STARS_70,
  ITUNES_STARS_80,
  ITUNES_STARS_90,
  ITUNES_STARS_100,
  ITUNES_STARS_MAX,
};

typedef struct itunes itunes_t;

bool  itunesConfigured (void);
itunes_t  *itunesAlloc (void);
void  itunesFree (itunes_t *itunes);
int   itunesGetStars (itunes_t *itunes, int idx);
void  itunesSetStars (itunes_t *itunes, int idx, int value);
void  itunesSaveStars (itunes_t *itunes);
int   itunesGetField (itunes_t *itunes, int idx);
void  itunesSetField (itunes_t *itunes, int idx, int value);
void  itunesSaveFields (itunes_t *itunes);
bool  itunesParse (itunes_t *itunes);

#endif /* INC_ITUNES_H */
