/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "datafile.h"
#include "ilist.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  DANCE_ANNOUNCE,           //
  DANCE_DANCE,              //
  DANCE_MPM_HIGH,           //
  DANCE_MPM_LOW,            //
  DANCE_SPEED,              //
  DANCE_TAGS,               //
  DANCE_TIMESIG,
  DANCE_TYPE,               //
  DANCE_KEY_MAX,
} dancekey_t;

/* keep ordering (same as datafile table) */
typedef enum {
  DANCE_SPEED_FAST,
  DANCE_SPEED_NORMAL,
  DANCE_SPEED_SLOW,
  DANCE_SPEED_MAX,
} dancespeed_t;

/* keep ordering (same as datafile table) */
typedef enum {
  DANCE_TIMESIG_24,
  DANCE_TIMESIG_34,
  DANCE_TIMESIG_44,
  DANCE_TIMESIG_MAX,
} dancetimesig_t;

enum {
  DANCE_NO_FORCE,
  DANCE_FORCE_CONV,
};

/* used by dance.c, uisongeditui.c, bdj4bpmcounter.c */
extern int danceTimesigValues [DANCE_TIMESIG_MAX];

typedef struct dance dance_t;

NODISCARD dance_t       *danceAlloc (const char *altfname);
void          danceFree (dance_t *);
void          danceStartIterator (dance_t *, slistidx_t *idx);
ilistidx_t    danceIterate (dance_t *, slistidx_t *idx);
ssize_t       danceGetCount (dance_t *);
const char *  danceGetStr (dance_t *, ilistidx_t dkey, ilistidx_t idx);
ssize_t       danceGetNum (dance_t *, ilistidx_t dkey, ilistidx_t idx);
slist_t       *danceGetList (dance_t *, ilistidx_t dkey, ilistidx_t idx);
void          danceSetStr (dance_t *, ilistidx_t dkey, ilistidx_t idx, const char *str);
void          danceSetNum (dance_t *, ilistidx_t dkey, ilistidx_t idx, ssize_t value);
void          danceSetList (dance_t *, ilistidx_t dkey, ilistidx_t idx, slist_t *list);
slist_t       *danceGetDanceList (dance_t *);
void          danceConvDance (datafileconv_t *conv);
void          danceSave (dance_t *dances, ilist_t *list, int newdistvers);
void          danceDelete (dance_t *dances, ilistidx_t dkey);
ilistidx_t    danceAdd (dance_t *dances, char *name);
int           danceGetTimeSignature (ilistidx_t danceIdx);
int           danceConvertBPMtoMPM (int danceidx, int bpm, int forceflag);
int           danceConvertMPMtoBPM (int danceidx, int bpm);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

