#ifndef INC_DANCE_H
#define INC_DANCE_H

#include "datafile.h"
#include "ilist.h"
#include "slist.h"

typedef enum {
  DANCE_ANNOUNCE,           //
  DANCE_DANCE,              //
  DANCE_HIGH_BPM,           //
  DANCE_LOW_BPM,            //
  DANCE_SPEED,              //
  DANCE_TAGS,               //
  DANCE_TIMESIG,
  DANCE_TYPE,               //
  DANCE_KEY_MAX,
} dancekey_t;

typedef enum {
  DANCE_SPEED_FAST,
  DANCE_SPEED_NORMAL,
  DANCE_SPEED_SLOW,
  DANCE_SPEED_MAX,
} dancespeed_t;

typedef enum {
  DANCE_TIMESIG_24,
  DANCE_TIMESIG_34,
  DANCE_TIMESIG_44,
  DANCE_TIMESIG_48,
  DANCE_TIMESIG_MAX,
} dancetimesig_t;

typedef struct dance dance_t;

dance_t       *danceAlloc (void);
void          danceFree (dance_t *);
void          danceStartIterator (dance_t *, ilistidx_t *idx);
ilistidx_t    danceIterate (dance_t *, ilistidx_t *idx);
ssize_t       danceGetCount (dance_t *);
char *        danceGetStr (dance_t *, ilistidx_t dkey, ilistidx_t idx);
ssize_t       danceGetNum (dance_t *, ilistidx_t dkey, ilistidx_t idx);
slist_t       *danceGetList (dance_t *, ilistidx_t dkey, ilistidx_t idx);
void          danceSetStr (dance_t *, ilistidx_t dkey, ilistidx_t idx, const char *str);
void          danceSetNum (dance_t *, ilistidx_t dkey, ilistidx_t idx, ssize_t value);
void          danceSetList (dance_t *, ilistidx_t dkey, ilistidx_t idx, slist_t *list);
slist_t       *danceGetDanceList (dance_t *);
void          danceConvDance (datafileconv_t *conv);
void          danceSave (dance_t *dances, ilist_t *list);
void          danceDelete (dance_t *dances, ilistidx_t dkey);
ilistidx_t    danceAdd (dance_t *dances, char *name);

#endif /* INC_DANCE_H */
