/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <math.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "datafile.h"
#include "fileop.h"
#include "mdebug.h"
#include "nlist.h"
#include "roman.h"
#include "song.h"
#include "songutil.h"
#include "tagdef.h"

void
songutilConvAdjustFlags (datafileconv_t *conv)
{
  if (conv->invt == VALUE_STR) {
    int         num;
    const char  *str;

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
      if (*str == SONG_ADJUST_STR_TEST) {
        num |= SONG_ADJUST_TEST;
      }
      ++str;
    }

    conv->outvt = VALUE_NUM;
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    int     num;
    char    tbuff [40];
    char    *str;

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
      if ((num & SONG_ADJUST_TEST) == SONG_ADJUST_TEST) {
        *str++ = SONG_ADJUST_STR_TEST;
      }
    }
    *str = '\0';

    conv->outvt = VALUE_STRVAL;
    conv->strval = mdstrdup (tbuff);
  }
}

void
songutilConvSongType (datafileconv_t *conv)
{
  if (conv->invt == VALUE_STR) {
    int         num;
    const char  *str;

    str = conv->str;

    num = SONG_TYPE_NORM;
    if (str != NULL) {
      if (strcmp (str, "remote") == 0) {
        num = SONG_TYPE_REMOTE;
      } else if (strcmp (str, "podcast") == 0) {
        num = SONG_TYPE_PODCAST;
      }
    }
    conv->outvt = VALUE_NUM;
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    int     num;
    char    *str;

    num = conv->num;
    switch (num) {
      case SONG_TYPE_NORM: {
        str = "normal";
        break;
      }
      case SONG_TYPE_REMOTE: {
        str = "remote";
        break;
      }
      case SONG_TYPE_PODCAST: {
        str = "podcast";
        break;
      }
      default: {
        str = "normal";
        break;
      }
    }
    conv->outvt = VALUE_STRVAL;
    conv->strval = mdstrdup (str);
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

void
songutilTitleFromWorkMovement (nlist_t *taglist)
{
  const char  *tval;
  char        tbuff [500];
  char        *tend = tbuff + sizeof (tbuff);
  char        rbuff [10];
  int         mnum;
  char        *p;

  if (bdjoptGetNum (OPT_G_USE_WORK_MOVEMENT) == false) {
    return;
  }

  tval = nlistGetStr (taglist, TAG_WORK);
  if (tval != NULL && *tval) {
    *tbuff = '\0';
    p = stpecpy (tbuff, tend, tval);

    tval = nlistGetStr (taglist, TAG_MOVEMENTNAME);
    if (tval != NULL && *tval) {
      p = stpecpy (p, tend, ": ");

      mnum = nlistGetNum (taglist, TAG_MOVEMENTNUM);
      if (mnum > 0) {
        convertToRoman (mnum, rbuff, sizeof (rbuff));
        p = stpecpy (p, tend, rbuff);
        p = stpecpy (p, tend, ". ");
      }

      p = stpecpy (p, tend, tval);
    }
    nlistSetStr (taglist, TAG_TITLE, tbuff);
  }
}
