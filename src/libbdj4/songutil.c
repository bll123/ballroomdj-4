/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "datafile.h"
#include "fileop.h"
#include "mdebug.h"
#include "songutil.h"

char *
songutilFullFileName (const char *sfname)
{
  char      *tname;
  size_t    len;

  if (sfname == NULL) {
    return NULL;
  }

  tname = mdmalloc (MAXPATHLEN);
  assert (tname != NULL);

  len = strlen (sfname);
  if ((len > 0 && sfname [0] == '/') ||
      (len > 2 && sfname [1] == ':' && sfname [2] == '/')) {
    strlcpy (tname, sfname, MAXPATHLEN);
  } else {
    snprintf (tname, MAXPATHLEN, "%s/%s",
        bdjoptGetStr (OPT_M_DIR_MUSIC), sfname);
  }
  return tname;
}

bool
songutilHasOriginal (const char *sfname)
{
  char      origfn [MAXPATHLEN];
  bool      exists = false;
  char      *fullfn;

  if (sfname == NULL) {
    return NULL;
  }

  fullfn = songutilFullFileName (sfname);
  snprintf (origfn, sizeof (origfn), "%s%s",
      fullfn, bdjvarsGetStr (BDJV_ORIGINAL_EXT));
  if (fileopFileExists (origfn)) {
    exists = true;
  }
  if (! exists) {
    snprintf (origfn, sizeof (origfn), "%s%s", fullfn, BDJ4_GENERIC_ORIG_EXT);
    if (fileopFileExists (origfn)) {
      exists = true;
    }
  }
  dataFree (fullfn);
  return exists;
}

void
songutilConvAdjustFlags (datafileconv_t *conv)
{
  if (conv->valuetype == VALUE_STR) {
    int   num;
    char  *str;

    str = conv->str;

    num = SONG_ADJUST_NONE;
    while (*str) {
      if (*str == SONG_ADJUST_STR_NORM) {
        num |= SONG_ADJUST_NORM;
      }
      if (*str == SONG_ADJUST_STR_TRIM) {
        num |= SONG_ADJUST_TRIM;
      }
      if (*str == SONG_ADJUST_STR_ADJUST) {
        num |= SONG_ADJUST_ADJUST;
      }
      ++str;
    }

    conv->valuetype = VALUE_NUM;
    if (conv->allocated) {
      mdfree (conv->str);
    }
    conv->num = num;
    conv->allocated = false;
  } else if (conv->valuetype == VALUE_NUM) {
    int   num;
    char  tbuff [40];
    char  *str;

    conv->valuetype = VALUE_STR;
    num = conv->num;

    *tbuff = '\0';
    str = tbuff;
    if (num > 0 && ! (num & SONG_ADJUST_INVALID)) {
      if ((num & SONG_ADJUST_ADJUST) == SONG_ADJUST_ADJUST) {
        *str++ = SONG_ADJUST_STR_ADJUST;
      }
      if ((num & SONG_ADJUST_TRIM) == SONG_ADJUST_TRIM) {
        *str++ = SONG_ADJUST_STR_TRIM;
      }
      if ((num & SONG_ADJUST_NORM) == SONG_ADJUST_NORM) {
        *str++ = SONG_ADJUST_STR_NORM;
      }
    }
    *str = '\0';

    conv->str = mdstrdup (tbuff);
    conv->allocated = true;
  }
}

ssize_t
songutilAdjustPosReal (ssize_t pos, int speed)
{
  double    drate;
  double    dpos;
  ssize_t   npos;

  if (speed < 0 || speed == 100) {
    return pos;
  }
  drate = (double) speed / 100.0;
  dpos = (double) pos / drate;
  npos = (ssize_t) round (dpos);
  return npos;
}

ssize_t
songutilNormalizePosition (ssize_t pos, int speed)
{
  double    drate;
  double    dpos;
  ssize_t   npos;

  if (speed < 0 || speed == 100) {
    return pos;
  }
  drate = (double) speed / 100.0;
  dpos = (double) pos * drate;
  npos = (ssize_t) round (dpos);
  return npos;
}

int
songutilAdjustBPM (int bpm, int speed)
{
  double    drate;
  double    dbpm;
  int       nbpm;

  if (speed < 0 || speed == 100) {
    return bpm;
  }
  drate = (double) speed / 100.0;
  dbpm = (double) bpm * drate;
  nbpm = (int) round (dbpm);
  return nbpm;
}

int
songutilNormalizeBPM (int bpm, int speed)
{
  double    drate;
  double    dbpm;
  ssize_t   nbpm;

  if (speed < 0 || speed == 100) {
    return bpm;
  }
  drate = (double) speed / 100.0;
  dbpm = (double) bpm / drate;
  nbpm = (int) round (dbpm);
  return nbpm;
}

