#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "bdj4.h"
#include "autosel.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "dancesel.h"
#include "ilist.h"
#include "log.h"
#include "nlist.h"
#include "playlist.h"
#include "queue.h"
#include "slist.h"

typedef struct {
  ilistidx_t    danceIdx;
} playedDance_t;

static bool     danceselProcessHistory (dancesel_t *dancesel,
                    ilistidx_t didx, ilistidx_t hdidx,
                    slist_t *tags, ssize_t *histCount);
static bool     matchTag (slist_t *tags, slist_t *otags);

dancesel_t *
danceselAlloc (nlist_t *countList)
{
  dancesel_t  *dancesel;
  ilistidx_t  didx;
  double      dcount = 0;
  double      dtemp;


  dancesel = malloc (sizeof (dancesel_t));
  assert (dancesel != NULL);
  dancesel->dances = bdjvarsdf [BDJVDF_DANCES];
  dancesel->autosel = bdjvarsdf [BDJVDF_AUTO_SEL];
  dcount = danceGetCount (dancesel->dances);
  dancesel->base = malloc (sizeof (danceProb_t) * dcount);
  dancesel->basetotal = 0.0;
  dancesel->distance = nlistAlloc ("dancesel-dist", LIST_ORDERED, NULL);
  dancesel->maxDistance = 0.0;
  dancesel->playedCounts = nlistAlloc ("dancesel-dist", LIST_ORDERED, NULL);
  dancesel->playedDances = queueAlloc (free);
  dancesel->adjustBase = NULL;
  dancesel->danceProbTable = NULL;
  dancesel->selCount = 0;

  /* autosel variables that will be used */
  dancesel->histDistance = autoselGetNum (dancesel->autosel, AUTOSEL_HIST_DISTANCE);
  dancesel->tagMatch = autoselGetDouble (dancesel->autosel, AUTOSEL_TAGMATCH);
  dancesel->tagAdjust = autoselGetDouble (dancesel->autosel, AUTOSEL_TAGADJUST);
  dancesel->logValue = autoselGetDouble (dancesel->autosel, AUTOSEL_LOG_VALUE);

  nlistStartIterator (countList);
  while ((didx = nlistIterateKey (countList)) >= 0) {
    ssize_t count = nlistGetDouble (countList, didx);
    nlistSetDouble (dancesel->base, didx, count);
    dancesel->basetotal += (double) count;
  }

  nlistStartIterator (dancesel->base);
  while ((didx = nlistIterateKey (dancesel->base)) >= 0) {
    dcount = nlistGetDouble (dancesel->base, didx);
    dtemp = floor (dancesel->basetotal / dcount);
    nlistSetDouble (dancesel->distance, didx, dtemp);
    dancesel->maxDistance =
        dtemp > dancesel->maxDistance ? dtemp : dancesel->maxDistance;
  }
  dancesel->maxDistance = (ssize_t) round (dancesel->maxDistance);

  return dancesel;
}

void
danceselFree (dancesel_t *dancesel)
{
  if (dancesel != NULL) {
    if (dancesel->base != NULL) {
      nlistFree (dancesel->base);
    }
    if (dancesel->distance != NULL) {
      nlistFree (dancesel->distance);
    }
    if (dancesel->playedCounts != NULL) {
      nlistFree (dancesel->playedCounts);
    }
    if (dancesel->playedDances != NULL) {
      queueFree (dancesel->playedDances);
    }
    if (dancesel->adjustBase != NULL) {
      nlistFree (dancesel->adjustBase);
    }
    if (dancesel->danceProbTable != NULL) {
      nlistFree (dancesel->danceProbTable);
    }
    free (dancesel);
  }
}

void
danceselAddPlayed (dancesel_t *dancesel, ilistidx_t danceIdx)
{
  double        count;
  playedDance_t *pd = NULL;

  if (dancesel == NULL || dancesel->playedDances == NULL) {
    return;
  }

  count = nlistGetDouble (dancesel->playedCounts, danceIdx);
  if (count == LIST_DOUBLE_INVALID) {
    count = 0.0;
  }
  nlistSetDouble (dancesel->playedCounts, danceIdx, count + 1.0);

  pd = malloc (sizeof (playedDance_t));
  pd->danceIdx = danceIdx;
  queuePushHead (dancesel->playedDances, pd);
}

ilistidx_t
danceselSelect (dancesel_t *dancesel, nlist_t *danceCounts,
    nlist_t *musicqList)
{
  ilistidx_t    didx;
  double        tbase;
  double        tprob;
  double        abase;
  playedDance_t *pd;
    /* expected counts */
  double        expcount;
  double        dcount;
    /* current dance values */
  slist_t       *tags = NULL;
  double        speed;
  double        type;
    /* previous dance data */
  ilistidx_t    pddanceIdx;
  slist_t       *pdtags = NULL;
  ssize_t       pdspeed;
  ssize_t       pdtype;
    /* history */
  ssize_t       histCount;
  ilistidx_t    hdidx;
  bool          hdone;
    /* probability calculation */
  double        adjTotal;
  double        tval;


  if (dancesel->adjustBase != NULL) {
    nlistFree (dancesel->adjustBase);
  }
  if (dancesel->danceProbTable != NULL) {
    nlistFree (dancesel->danceProbTable);
  }
  dancesel->adjustBase = nlistAlloc ("dancesel-adjust-base", LIST_ORDERED, NULL);
  dancesel->danceProbTable = nlistAlloc ("dancesel-adjust-base", LIST_ORDERED, NULL);

  /* data from the previoous dance */
  /* use the dance at the bottom of the music queue (top of musicqList) */
  pddanceIdx = nlistGetNumByIdx (musicqList, 0);
  if (pddanceIdx >= 0) {
    pdspeed = danceGetNum (dancesel->dances, pddanceIdx, DANCE_SPEED);
    pdtags = danceGetList (dancesel->dances, pddanceIdx, DANCE_TAGS);
    pdtype = danceGetNum (dancesel->dances, pddanceIdx, DANCE_TYPE);
  }

  nlistStartIterator (dancesel->base);
  while ((didx = nlistIterateKey (dancesel->base)) >= 0) {
    tbase = nlistGetNum (dancesel->base, didx);
    tprob = tbase / dancesel->basetotal;
    abase = tbase;

    /* are any dances of this type still available ? */
    dcount = nlistGetNum (danceCounts, didx);
    if (dcount <= 0) {
      abase = 0.0;
      nlistSetDouble (dancesel->adjustBase, didx, abase);
      logMsg (LOG_DBG, LOG_DANCESEL, "no more %ld left", didx);
      /* no need to do the other checks */
      continue;
    }

    expcount = fmax (1.0, round (tprob * dancesel->totalPlayed + 0.5));
    dcount = nlistGetDouble (dancesel->playedCounts, didx);
    speed = danceGetNum (dancesel->dances, didx, DANCE_SPEED);

    /* expected count high ( / 800 ) */
    if (dcount > expcount) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_LIMIT);
      logMsg (LOG_DBG, LOG_DANCESEL, "dcount %ld > expcount %ld", dcount, expcount);
    }

    /* expected count low ( / 0.5 ) */
    if (dcount < expcount - 1) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_LOW);
      logMsg (LOG_DBG, LOG_DANCESEL, "dcount %ld < expcount %ld", dcount, expcount - 1);
    }

    /* if this selection is at the beginning of the playlist */
    if (dancesel->selCount < autoselGetNum (dancesel->autosel, AUTOSEL_BEG_COUNT)) {
      /* if this dance is a fast dance ( / 1000) */
      if (speed == DANCE_SPEED_FAST) {
        abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_BEG_FAST);
        logMsg (LOG_DBG, LOG_DANCESEL, "fast / begin of playlist");
      }
    }

    /* if this dance and the previous dance were both fast ( / 1000) */
    if (pddanceIdx >= 0 && pdspeed == speed && speed == DANCE_SPEED_FAST) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_BOTHFAST);
      logMsg (LOG_DBG, LOG_DANCESEL, "speed is fast and same as previous");
    }

    /* if there is a tag match between the previous dance and this one */
    /* ( / 600 ) */
    tags = danceGetList (dancesel->dances, didx, DANCE_TAGS);
    if (pddanceIdx >= 0 && matchTag (tags, pdtags)) {
      abase = abase / dancesel->tagMatch;
      logMsg (LOG_DBG, LOG_DANCESEL, "matched tags with previous");
    }

    /* if this dance and the previous dance have matching types ( / 600 ) */
    type = danceGetNum (dancesel->dances, didx, DANCE_TYPE);
    if (pddanceIdx >= 0 && pdtype == type) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_TYPE_MATCH);
      logMsg (LOG_DBG, LOG_DANCESEL, "matched type with previous");
    }

    nlistSetDouble (dancesel->adjustBase, didx, abase);

    if (musicqList != NULL) {
      /* now go back through the history */
      nlistStartIterator (musicqList);
      nlistIterateKey (musicqList);     // previous dance has already been used

      queueStartIterator (dancesel->playedDances);

      histCount = 1;
      hdone = false;
      while (! hdone && (hdidx = nlistIterateKey (musicqList)) >= 0) {
        hdone = danceselProcessHistory (dancesel, didx, hdidx,
            tags, &histCount);
      } /* for each of the played dances */
      while (! hdone && (pd = queueIterateData (dancesel->playedDances)) != NULL) {
        hdidx = pd->danceIdx;
        hdone = danceselProcessHistory (dancesel, didx, hdidx,
            tags, &histCount);
      } /* for each of the played dances */
    } /* if there is history to be checked */
  } /* for each dance in the base list */

  /* get the totals for the adjusted base values */
  adjTotal = 0;
  nlistStartIterator (dancesel->base);
  while ((didx = nlistIterateKey (dancesel->base)) >= 0) {
    abase = nlistGetDouble (dancesel->adjustBase, didx);
    adjTotal += abase;
  }

  /* create a probability table of running totals */
  tprob = 0.0;
  nlistStartIterator (dancesel->base);
  while ((didx = nlistIterateKey (dancesel->base)) >= 0) {
    abase = nlistGetDouble (dancesel->adjustBase, didx);
    tval = abase / adjTotal;
    tprob += tval;
    nlistSetDouble (dancesel->danceProbTable, didx, tprob);
  }

  /* ### copy the searchforpercentage routine from songsel and rework it */

  return 0;
}

/* internal routines */

static bool
danceselProcessHistory (dancesel_t *dancesel, ilistidx_t didx,
    ilistidx_t hdidx, slist_t *tags, ssize_t *histCount)
{
  /* working with the history (played dances) */
  slist_t       *htags = NULL;
  double        dist;
  double        tval;
  double        findex;
  double        abase;


  /* the tags of the first few dances in the history are checked */
  if (*histCount <= dancesel->histDistance) {
    htags = danceGetList (dancesel->dances, hdidx, DANCE_TAGS);
    if (matchTag (tags, htags)) {
      tval = pow (dancesel->tagAdjust, (double) *histCount);
      abase = nlistGetDouble (dancesel->adjustBase, didx);
      abase = abase / dancesel->tagMatch * tval;
      nlistSetDouble (dancesel->adjustBase, didx, abase);
      logMsg (LOG_DBG, LOG_DANCESEL, "matched tags with history");
    }
  }

  if (hdidx != didx) {
    (*histCount)++;
    if (*histCount > dancesel->maxDistance) {
      return true;
    }
    return false;
  }

  /* found the matching dance */

  dist = nlistGetDouble (dancesel->distance, hdidx);
  tval = (double) (*histCount - 1);

  /* if the dance hasn't appeared within 'dist', then no adjustment */
  /* is needed */
  if (dist < tval || abase == 0.0) {
    (*histCount)++;
    if (*histCount > dancesel->maxDistance ||
        *histCount > dancesel->histDistance) {
      /* no more history needs to be checked */
      return true;
    }
    return false;
  }

  /* logarithmic */
  findex = fmax (0.0, fmin (1.0, tval / dist));
  findex = 1.0 - (pow (0.1, (1.0 - findex) * dancesel->logValue));

  abase = nlistGetDouble (dancesel->adjustBase, hdidx);
  abase = fmax (0.0, abase - (findex * abase));
  logMsg (LOG_DBG, LOG_DANCESEL, "adjust due to distance");
  nlistSetDouble (dancesel->adjustBase, hdidx, abase);

  (*histCount)++;
  /* having reached this point, the dance has been found  */
  /* the matching dance history processing is finished    */
  /* the tag history check also needs to be finished      */
  if (*histCount > dancesel->histDistance) {
    return true;
  }

  return false;
}


static bool
matchTag (slist_t *tags, slist_t *otags)
{
  char      *ttag;
  char      *otag;

  if (tags != NULL && otags != NULL) {
    slistStartIterator (tags);
    while ((ttag = slistIterateKey (tags)) != NULL) {
      slistStartIterator (otags);
      while ((otag = slistIterateKey (otags)) != NULL) {
        if (strcmp (ttag, otag) == 0) {
          return true;
        }
      } /* for each tag for the previous dance */
    } /* for each tag for this dance */
  } /* if both dances have tags */
  return false;
}
