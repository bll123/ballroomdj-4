/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

typedef struct uirating {
  rating_t      *ratings;
  uispinbox_t   *spinbox;
  bool          allflag;
} uirating_t;

static const char *uiratingRatingGet (void *udata, int idx);

uirating_t *
uiratingSpinboxCreate (uiwcont_t *boxp, bool allflag)
{
  uirating_t  *uirating;
  int         maxw;
  int         start;
  int         len;


  uirating = mdmalloc (sizeof (uirating_t));
  uirating->ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uirating->allflag = allflag;
  uirating->spinbox = uiSpinboxInit ();

  uiSpinboxTextCreate (uirating->spinbox, uirating);

  start = 0;
  maxw = ratingGetMaxWidth (uirating->ratings);
  if (allflag) {
    /* CONTEXT: rating: a filter: all dance ratings will be listed */
    len = istrlen (_("All Ratings"));
    if (len > maxw) {
      maxw = len;
    }
    start = -1;
  }
  uiSpinboxTextSet (uirating->spinbox, start,
      ratingGetCount (uirating->ratings),
      maxw, NULL, NULL, uiratingRatingGet);

  uiBoxPackStart (boxp, uiSpinboxGetWidgetContainer (uirating->spinbox));

  return uirating;
}


void
uiratingFree (uirating_t *uirating)
{
  if (uirating != NULL) {
    uiSpinboxFree (uirating->spinbox);
    mdfree (uirating);
  }
}

int
uiratingGetValue (uirating_t *uirating)
{
  int   idx;

  if (uirating == NULL) {
    return 0;
  }

  idx = uiSpinboxTextGetValue (uirating->spinbox);
  return idx;
}

void
uiratingSetValue (uirating_t *uirating, int value)
{
  if (uirating == NULL) {
    return;
  }

  uiSpinboxTextSetValue (uirating->spinbox, value);
}

void
uiratingSetState (uirating_t *uirating, int state)
{
  if (uirating == NULL || uirating->spinbox == NULL) {
    return;
  }
  uiSpinboxSetState (uirating->spinbox, state);
}

void
uiratingSizeGroupAdd (uirating_t *uirating, uiwcont_t *sg)
{
  uiSizeGroupAdd (sg, uiSpinboxGetWidgetContainer (uirating->spinbox));
}

void
uiratingSetChangedCallback (uirating_t *uirating, callback_t *cb)
{
  uiSpinboxTextSetValueChangedCallback (uirating->spinbox, cb);
}

/* internal routines */

static const char *
uiratingRatingGet (void *udata, int idx)
{
  uirating_t  *uirating = udata;

  if (idx == -1) {
    if (uirating->allflag) {
      /* CONTEXT: rating: a filter: all dance ratings are displayed in the song selection */
      return _("All Ratings");
    }
    idx = 0;
  }
  return ratingGetRating (uirating->ratings, idx);
}

