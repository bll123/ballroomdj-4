/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "datafile.h"
#include "dnctypes.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "slist.h"

int danceTimesigValues [DANCE_TIMESIG_MAX] = {
  [DANCE_TIMESIG_24] = 2,
  [DANCE_TIMESIG_34] = 3,
  [DANCE_TIMESIG_44] = 4,
};

typedef struct dance {
  datafile_t      *df;
  ilist_t         *dances;
  slist_t         *danceList;
  char            *path;
} dance_t;

enum {
  DANCE_BPM_VERSION = 1,
  DANCE_DF_VERSION = 2,
};

static void danceConvSpeed (datafileconv_t *conv);
static void danceConvTimeSig (datafileconv_t *conv);
static void danceCreateDanceList (dance_t *dances);

/* must be sorted in ascii order */
static datafilekey_t dancedfkeys [] = {
  { "ANNOUNCE",   DANCE_ANNOUNCE, VALUE_STR, NULL, DF_NORM },
  { "DANCE",      DANCE_DANCE,    VALUE_STR, NULL, DF_NORM },
  { "HIGHBPM",    DANCE_MPM_HIGH, VALUE_NUM, NULL, DF_NO_WRITE }, // version 1
  { "HIGHMPM",    DANCE_MPM_HIGH, VALUE_NUM, NULL, DF_NORM },
  { "LOWBPM",     DANCE_MPM_LOW,  VALUE_NUM, NULL, DF_NO_WRITE }, // version 1
  { "LOWMPM",     DANCE_MPM_LOW,  VALUE_NUM, NULL, DF_NORM },
  { "SPEED",      DANCE_SPEED,    VALUE_NUM, danceConvSpeed, DF_NORM },
  { "TAGS",       DANCE_TAGS,     VALUE_LIST, convTextList, DF_NORM },
  { "TIMESIG",    DANCE_TIMESIG,  VALUE_NUM, danceConvTimeSig, DF_NORM },
  { "TYPE",       DANCE_TYPE,     VALUE_NUM, dnctypesConv, DF_NORM },
};
enum {
  dancedfcount = sizeof (dancedfkeys) / sizeof (datafilekey_t),
};

static datafilekey_t dancespeeddfkeys [DANCE_SPEED_MAX] = {
  { "fast",       DANCE_SPEED_FAST,   VALUE_NUM, NULL, DF_NORM },
  { "normal",     DANCE_SPEED_NORMAL, VALUE_NUM, NULL, DF_NORM },
  { "slow",       DANCE_SPEED_SLOW,   VALUE_NUM, NULL, DF_NORM },
};

static datafilekey_t dancetimesigdfkeys [DANCE_TIMESIG_MAX] = {
  { "2/4",       DANCE_TIMESIG_24,   VALUE_NUM, NULL, DF_NORM },
  { "3/4",       DANCE_TIMESIG_34,   VALUE_NUM, NULL, DF_NORM },
  { "4/4",       DANCE_TIMESIG_44,   VALUE_NUM, NULL, DF_NORM },
};

dance_t *
danceAlloc (const char *altfname)
{
  dance_t     *dances;
  char        fname [MAXPATHLEN];

  if (altfname != NULL) {
    strlcpy (fname, altfname, sizeof (fname));
  } else {
    pathbldMakePath (fname, sizeof (fname), DANCE_FN,
        BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  }
  if (! fileopFileExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "dance: missing: %s", fname);
    return NULL;
  }

  dances = mdmalloc (sizeof (dance_t));

  dances->path = mdstrdup (fname);
  dances->danceList = NULL;

  dances->df = datafileAllocParse ("dance", DFTYPE_INDIRECT, fname,
      dancedfkeys, dancedfcount, DF_NO_OFFSET, NULL);
  dances->dances = datafileGetList (dances->df);

  danceCreateDanceList (dances);

  return dances;
}

void
danceFree (dance_t *dances)
{
  if (dances == NULL) {
    return;
  }

  dataFree (dances->path);
  datafileFree (dances->df);
  slistFree (dances->danceList);
  mdfree (dances);
}

void
danceStartIterator (dance_t *dances, slistidx_t *iteridx)
{
  if (dances == NULL || dances->dances == NULL) {
    return;
  }

  /* use the dancelist so that the return is always sorted */
  slistStartIterator (dances->danceList, iteridx);
}

ilistidx_t
danceIterate (dance_t *dances, slistidx_t *iteridx)
{
  ilistidx_t     ikey;

  if (dances == NULL || dances->dances == NULL) {
    return LIST_LOC_INVALID;
  }

  ikey = slistIterateValueNum (dances->danceList, iteridx);
  return ikey;
}

ssize_t
danceGetCount (dance_t *dances)
{
  if (dances == NULL || dances->dances == NULL) {
    return 0;
  }
  return ilistGetCount (dances->dances);
}

const char *
danceGetStr (dance_t *dances, ilistidx_t dkey, ilistidx_t idx)
{
  if (dances == NULL || dances->dances == NULL) {
    return NULL;
  }
  return ilistGetStr (dances->dances, dkey, idx);
}

slist_t *
danceGetList (dance_t *dances, ilistidx_t dkey, ilistidx_t idx)
{
  if (dances == NULL || dances->dances == NULL) {
    return NULL;
  }
  return ilistGetList (dances->dances, dkey, idx);
}

ssize_t
danceGetNum (dance_t *dances, ilistidx_t dkey, ilistidx_t idx)
{
  if (dances == NULL || dances->dances == NULL) {
    return LIST_VALUE_INVALID;
  }
  return ilistGetNum (dances->dances, dkey, idx);
}

void
danceSetStr (dance_t *dances, ilistidx_t dkey, ilistidx_t idx, const char *str)
{
  if (dances == NULL || dances->dances == NULL) {
    return;
  }
  ilistSetStr (dances->dances, dkey, idx, str);
}

void
danceSetNum (dance_t *dances, ilistidx_t dkey, ilistidx_t idx, ssize_t value)
{
  if (dances == NULL || dances->dances == NULL) {
    return;
  }
  ilistSetNum (dances->dances, dkey, idx, value);
}

void
danceSetList (dance_t *dances, ilistidx_t dkey, ilistidx_t idx, slist_t *list)
{
  if (dances == NULL || dances->dances == NULL) {
    return;
  }
  ilistSetList (dances->dances, dkey, idx, list);
}

slist_t *
danceGetDanceList (dance_t *dances)
{
  if (dances == NULL || dances->dances == NULL) {
    return NULL;
  }
  return dances->danceList;
}

void
danceConvDance (datafileconv_t *conv)
{
  dance_t   *dances;
  ssize_t   num;

  if (conv == NULL) {
    return;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;
    num = slistGetNum (dances->danceList, conv->str);
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    num = conv->num;
    conv->str = ilistGetStr (dances->dances, num, DANCE_DANCE);
  }
}

void
danceSave (dance_t *dances, ilist_t *list, int newdistvers)
{
  int   distvers;

  if (dances == NULL) {
    return;
  }
  if (list == NULL) {
    list = dances->dances;
  }
  distvers = datafileDistVersion (dances->df);
  if (newdistvers > distvers) {
    distvers = newdistvers;
  }
  datafileSave (dances->df, NULL, list, DF_NO_OFFSET, distvers);
  danceCreateDanceList (dances);
}

void
danceDelete (dance_t *dances, ilistidx_t dkey)
{
  const char  *val;

  val = ilistGetStr (dances->dances, dkey, DANCE_DANCE);
  slistDelete (dances->danceList, val);
  ilistDelete (dances->dances, dkey);
}

ilistidx_t
danceAdd (dance_t *dances, char *name)
{
  ilistidx_t    count;

  count = ilistGetCount (dances->dances);
  ilistSetStr (dances->dances, count, DANCE_DANCE, name);
  ilistSetNum (dances->dances, count, DANCE_SPEED, DANCE_SPEED_NORMAL);
  ilistSetNum (dances->dances, count, DANCE_TYPE, 0);
  ilistSetNum (dances->dances, count, DANCE_TIMESIG, DANCE_TIMESIG_44);
  ilistSetNum (dances->dances, count, DANCE_MPM_HIGH, 0);
  ilistSetNum (dances->dances, count, DANCE_MPM_LOW, 0);
  ilistSetStr (dances->dances, count, DANCE_ANNOUNCE, "");
  ilistSetList (dances->dances, count, DANCE_TAGS, NULL);
  slistSetNum (dances->danceList, name, count);
  return count;
}

int
danceGetTimeSignature (ilistidx_t danceIdx)
{
  dance_t     *dances;
  int         timesig;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  if (danceIdx < 0) {
    /* unknown / not-set dance */
    timesig = DANCE_TIMESIG_44;
  } else {
    timesig = danceGetNum (dances, danceIdx, DANCE_TIMESIG);
  }
  if (timesig < 0) {
    timesig = DANCE_TIMESIG_44;
  }

  return timesig;
}

/* the forceflag is used by bdj4updater */
int
danceConvertBPMtoMPM (int danceidx, int bpm, int forceflag)
{
  if (bpm > 0 &&
      (bdjoptGetNum (OPT_G_BPM) == BPM_BPM ||
      forceflag == DANCE_FORCE_CONV)) {
    int   timesig;

    timesig = danceGetTimeSignature (danceidx);
    bpm /= danceTimesigValues [timesig];
  }

  return bpm;
}

int
danceConvertMPMtoBPM (int danceidx, int bpm)
{
  if (bdjoptGetNum (OPT_G_BPM) == BPM_BPM) {
    int   timesig;

    timesig = danceGetTimeSignature (danceidx);
    bpm *= danceTimesigValues [timesig];
  }

  return bpm;
}

int
danceGetDistVersion (dance_t *dances)
{
  int   distvers = 0;

  if (dances == NULL) {
    return 0;
  }

  distvers = datafileDistVersion (dances->df);
  return distvers;
}



/* internal routines */

static void
danceConvSpeed (datafileconv_t *conv)
{
  nlistidx_t  idx;
  const char  *sval;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;
    sval = conv->str;
    idx = dfkeyBinarySearch (dancespeeddfkeys, DANCE_SPEED_MAX, sval);
    conv->num = LIST_VALUE_INVALID;
    if (idx >= 0) {
      conv->num = dancespeeddfkeys [idx].itemkey;
    }
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    if (conv->num < 0 || conv->num >= DANCE_SPEED_MAX) {
      sval = dancespeeddfkeys [DANCE_SPEED_NORMAL].name;  // unknown -> normal
    } else {
      sval = dancespeeddfkeys [conv->num].name;
    }
    conv->str = sval;
  }
}

static void
danceConvTimeSig (datafileconv_t *conv)
{
  nlistidx_t       idx;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;
    idx = dfkeyBinarySearch (dancetimesigdfkeys, DANCE_TIMESIG_MAX, conv->str);
    conv->num = LIST_VALUE_INVALID;
    if (idx >= 0) {
      conv->num = dancetimesigdfkeys [idx].itemkey;
    } else {
      /* in case there are any leftover 4/8 settings */
      conv->num = DANCE_TIMESIG_44;
    }
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    if (conv->num < 0 || conv->num >= DANCE_TIMESIG_MAX) {
      conv->str = NULL;
    } else {
      conv->str = dancetimesigdfkeys [conv->num].name;
    }
  }
}

static void
danceCreateDanceList (dance_t *dances)
{
  ilistidx_t  iteridx;
  int         key;

  slistFree (dances->danceList);
  dances->danceList = slistAlloc ("dance-list", LIST_UNORDERED, NULL);
  slistSetSize (dances->danceList, ilistGetCount (dances->dances));

  ilistStartIterator (dances->dances, &iteridx);
  while ((key = ilistIterateKey (dances->dances, &iteridx)) >= 0) {
    const char  *val;

    val = ilistGetStr (dances->dances, key, DANCE_DANCE);
    slistSetNum (dances->danceList, val, key);
  }
  slistSort (dances->danceList);
}
