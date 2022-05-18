#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "rating.h"
#include "ui.h"
#include "uirating.h"
#include "uiutils.h"

typedef struct uirating {
  rating_t      *ratings;
  uispinbox_t   spinbox;
  bool          allflag;
} uirating_t;

static char *uiratingRatingGet (void *udata, int idx);

uirating_t *
uiratingSpinboxCreate (UIWidget *boxp, bool allflag)
{
  uirating_t  *uirating;
  GtkWidget   *widget;
  int         maxw;
  int         start;
  int         len;


  uirating = malloc (sizeof (uirating_t));
  uirating->ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uirating->allflag = allflag;
  uiSpinboxTextInit (&uirating->spinbox);

  widget = uiSpinboxTextCreate (&uirating->spinbox, uirating);

  start = 0;
  maxw = ratingGetMaxWidth (uirating->ratings);
  if (allflag) {
    /* CONTEXT: a filter: all dance ratings will be listed */
    len = istrlen (_("All Ratings"));
    if (len > maxw) {
      maxw = len;
    }
    start = -1;
  }
  uiSpinboxTextSet (&uirating->spinbox, start,
      ratingGetCount (uirating->ratings),
      maxw, NULL, NULL, uiratingRatingGet);

  uiBoxPackStartUW (boxp, widget);

  return uirating;
}


void
uiratingFree (uirating_t *uirating)
{
  if (uirating != NULL) {
    uiSpinboxTextFree (&uirating->spinbox);
    free (uirating);
  }
}

int
uiratingGetValue (uirating_t *uirating)
{
  int   idx;

  idx = uiSpinboxTextGetValue (&uirating->spinbox);
  return idx;
}

/* internal routines */

static char *
uiratingRatingGet (void *udata, int idx)
{
  uirating_t  *uirating = udata;

  if (idx == -1) {
    if (uirating->allflag) {
      /* CONTEXT: a filter: all dance ratings are displayed in the song selection */
      return _("All Ratings");
    }
    idx = 0;
  }
  return ratingGetRating (uirating->ratings, idx);
}

