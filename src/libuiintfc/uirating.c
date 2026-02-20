/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "istring.h"
#include "mdebug.h"
#include "rating.h"
#include "ui.h"
#include "uirating.h"
#include "uisbtext.h"

typedef struct uirating {
  rating_t    *ratings;
  uisbtext_t  *sb;
  bool        allflag;
} uirating_t;

static const char *uiratingRatingGet (void *udata, int idx);

uirating_t *
uiratingSpinboxCreate (uiwcont_t *boxp, bool allflag)
{
  uirating_t  *uirating;
  int         maxw;


  uirating = mdmalloc (sizeof (uirating_t));
  uirating->ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uirating->allflag = allflag;
  uirating->sb = uisbtextCreate (boxp);

  maxw = ratingGetMaxWidth (uirating->ratings);
  if (allflag == UIRATING_ALL) {
    const char  *txt;
    int         len;

    /* CONTEXT: rating: a filter: all dance ratings will be listed */
    txt = _("All Ratings");
    len = istrlen (txt);
    if (len > maxw) {
      maxw = len;
    }
    uisbtextPrependList (uirating->sb, txt);
  }
  uisbtextSetDisplayCallback (uirating->sb, uiratingRatingGet, uirating);
  uisbtextSetCount (uirating->sb, ratingGetCount (uirating->ratings));
  uisbtextSetWidth (uirating->sb, maxw);

  return uirating;
}


void
uiratingFree (uirating_t *uirating)
{
  if (uirating == NULL) {
    return;
  }

  uisbtextFree (uirating->sb);
  mdfree (uirating);
}

int
uiratingGetValue (uirating_t *uirating)
{
  int   idx;

  if (uirating == NULL) {
    return 0;
  }

  idx = uisbtextGetValue (uirating->sb);
  return idx;
}

void
uiratingSetValue (uirating_t *uirating, int value)
{
  if (uirating == NULL) {
    return;
  }

  uisbtextSetValue (uirating->sb, value);
}

void
uiratingSetState (uirating_t *uirating, int state)
{
  if (uirating == NULL || uirating->sb == NULL) {
    return;
  }
  uisbtextSetState (uirating->sb, state);
}

void
uiratingSizeGroupAdd (uirating_t *uirating, uiwcont_t *sg)
{
  uisbtextSizeGroupAdd (uirating->sb, sg);
}

void
uiratingSetChangedCallback (uirating_t *uirating, callback_t *cb)
{
  uisbtextSetChangeCallback (uirating->sb, cb);
}

/* internal routines */

static const char *
uiratingRatingGet (void *udata, int idx)
{
  uirating_t  *uirating = udata;

  if (idx == -1) {
    if (uirating->allflag == UIRATING_ALL) {
      /* CONTEXT: rating: a filter: all dance ratings are displayed in the song selection */
      return _("All Ratings");
    }
    idx = 0;
  }
  return ratingGetRating (uirating->ratings, idx);
}

