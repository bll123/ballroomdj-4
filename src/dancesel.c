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
#include "nlist.h"
#include "playlist.h"
#include "queue.h"
#include "slist.h"

typedef struct {
  nlistidx_t    danceKey;
} playedDance_t;

static bool     matchTag (slist_t *tags, slist_t *otags);

dancesel_t *
danceselAlloc (playlist_t *pl)
{
  dancesel_t  *dancesel;
  nlistidx_t  dkey;
  ssize_t     sel;
  ssize_t     count;
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

  danceStartIterator (dancesel->dances);
  while ((dkey = danceIterateKey (dancesel->dances)) >= 0) {
    sel = playlistGetDanceNum (pl, dkey, PLDANCE_SELECTED);
    if (sel) {
      count = playlistGetDanceNum (pl, dkey, PLDANCE_COUNT);
      nlistSetDouble (dancesel->base, dkey, (double) count);
      dancesel->basetotal += (double) count;
    }
  }

  nlistStartIterator (dancesel->base);
  while ((dkey = nlistIterateKey (dancesel->base)) >= 0) {
    dcount = nlistGetDouble (dancesel->base, dkey);
    dtemp = floor (dancesel->basetotal / dcount);
    nlistSetDouble (dancesel->distance, dkey, dtemp);
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
danceselAddPlayed (dancesel_t *dancesel, nlistidx_t danceKey)
{
  double        count;
  playedDance_t *pd = NULL;

  if (dancesel == NULL || dancesel->playedDances == NULL) {
    return;
  }

  count = nlistGetDouble (dancesel->playedCounts, danceKey);
  if (count == LIST_DOUBLE_INVALID) {
    count = 0.0;
  }
  nlistSetDouble (dancesel->playedCounts, danceKey, count + 1.0);

  pd = malloc (sizeof (playedDance_t));
  pd->danceKey = danceKey;
  queuePushHead (dancesel->playedDances, pd);
}

nlistidx_t
danceselSelect (dancesel_t *dancesel,
    danceselCallback_t callback, void *userdata)
{
  nlistidx_t     dkey;
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
  nlistidx_t    pdanceKey;
  slist_t       *pdtags = NULL;
  ssize_t       pdspeed;
  ssize_t       pdtype;
    /* working with the history (played dances) */
  slist_t       *htags = NULL;
  double        dist;
  double        tval;
  nlistidx_t    hdkey;
  ssize_t       histCount;
  double        findex;
    /* autosel variables that will be used */
  ssize_t       histDistance;
  double        tagAdjust;
  double        tagMatch;
  double        logValue;
    /* probability calculation */
  double        adjTotal;


  if (dancesel->adjustBase != NULL) {
    nlistFree (dancesel->adjustBase);
  }
  if (dancesel->danceProbTable != NULL) {
    nlistFree (dancesel->danceProbTable);
  }
  dancesel->adjustBase = nlistAlloc ("dancesel-adjust-base", LIST_ORDERED, NULL);
  dancesel->danceProbTable = nlistAlloc ("dancesel-adjust-base", LIST_ORDERED, NULL);

    /* data from the previoous dance */
  pd = queueGetCurrent (dancesel->playedDances);
  pdanceKey = pd->danceKey;
  pdspeed = danceGetNum (dancesel->dances, pdanceKey, DANCE_SPEED);
  pdtags = danceGetList (dancesel->dances, pdanceKey, DANCE_TAGS);
  pdtype = danceGetNum (dancesel->dances, pdanceKey, DANCE_TYPE);

    /* autosel variables that will be used */
  histDistance = autoselGetNum (dancesel->autosel, AUTOSEL_HIST_DISTANCE);
  tagMatch = autoselGetDouble (dancesel->autosel, AUTOSEL_TAGMATCH);
  tagAdjust = autoselGetDouble (dancesel->autosel, AUTOSEL_TAGADJUST);
  logValue = autoselGetDouble (dancesel->autosel, AUTOSEL_LOG_VALUE);

  nlistStartIterator (dancesel->base);
  while ((dkey = nlistIterateKey (dancesel->base)) >= 0) {
    tbase = nlistGetNum (dancesel->base, dkey);
    tprob = tbase / dancesel->basetotal;

    expcount = fmax (1.0, round (tprob * dancesel->totalPlayed + 0.5));
    dcount = nlistGetDouble (dancesel->playedCounts, dkey);
    speed = danceGetNum (dancesel->dances, dkey, DANCE_SPEED);

    /* expected count high ( / 800 ) */
    if (dcount > expcount) {
      abase = tbase / autoselGetDouble (dancesel->autosel, AUTOSEL_LIMIT);
    }

    /* are any dances of this type still available ? */
// ### FIX : need to check with main, or get this data passed in.
//  note that if we're creating a mix from a songlist, this would be
//  different.

    /* expected count low ( / 0.5 ) */
    if (dcount < expcount - 1) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_LOW);
    }

    /* if this selection is at the beginning of the playlist */
    if (dancesel->selCount < autoselGetNum (dancesel->autosel, AUTOSEL_BEG_COUNT)) {
      /* if this dance is a fast dance ( / 1000) */
      if (speed == DANCE_SPEED_FAST) {
        abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_BEG_FAST);
      }
    }

    /* if this dance and the previous dance were both fast ( / 1000) */
    if (pdspeed == speed && speed == DANCE_SPEED_FAST) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_BOTHFAST);
    }

    /* if there is a tag match between the previous dance and this one */
    /* ( / 600 ) */
    tags = danceGetList (dancesel->dances, dkey, DANCE_TAGS);
    if (matchTag (tags, pdtags)) {
      abase = abase / tagMatch;
    }

    /* if this dance and the previous dance have matching types ( / 600 ) */
    type = danceGetNum (dancesel->dances, dkey, DANCE_TYPE);
    if (pdtype == type) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_TYPE_MATCH);
    }

    nlistSetDouble (dancesel->adjustBase, dkey, abase);

/* ### FIX review bdj3 code, figure out how the callback (to-be-played) works */
/* in conjunction with the played list */
    if (1) {
      /* now go back through the history */
      queueStartIterator (dancesel->playedDances);
      queueIterateData (dancesel->playedDances);
      histCount = 1;
      while ((pd = queueIterateData (dancesel->playedDances)) != NULL) {
        hdkey = pd->danceKey;

        /* the tags of the first few dances in the history are checked */
        if (histCount <= histDistance) {
          htags = danceGetList (dancesel->dances, hdkey, DANCE_TAGS);
          if (matchTag (tags, htags)) {
            tval = pow (tagAdjust, (double) histCount);
            abase = abase / tagMatch * tval;
            nlistSetDouble (dancesel->adjustBase, dkey, abase);
          }
        }

        if (hdkey != dkey) {
          ++histCount;
          if (histCount > dancesel->maxDistance) {
            break;
          }
          continue;
        }

        /* found the matching dance */

        dist = nlistGetDouble (dancesel->distance, hdkey);
        tval = (double) (histCount - 1);

        /* if the dance hasn't appeared within 'dist', then no adjustment */
        /* is needed */
        if (dist < tval || abase == 0.0) {
          ++histCount;
          if (histCount > dancesel->maxDistance) {
            break;
          }
          continue;
        }

        /* logarithmic */
        findex = fmax (0.0, fmin (1.0, tval / dist));
        findex = 1.0 - (pow (0.1, (1.0 - findex) * logValue));

        abase = nlistGetDouble (dancesel->adjustBase, hdkey);
        abase = fmax (0.0, abase - (findex * abase));
        nlistSetDouble (dancesel->adjustBase, hdkey, abase);

        ++histCount;
        /* having reached this point, the dance has been found  */
        /* the matching dance history processing is finished    */
        /* the tag history check also needs to be finished      */
        if (histCount > histDistance) {
          break;
        }
      } /* for each of the played dances */
    } /* if there is history to be checked */
  } /* for each dance in the base list */

  /* get the totals for the adjusted base values */
  adjTotal = 0;
  nlistStartIterator (dancesel->base);
  while ((dkey = nlistIterateKey (dancesel->base)) >= 0) {
    abase = nlistGetDouble (dancesel->adjustBase, dkey);
    adjTotal += abase;
  }

  /* create a probability table of running totals */
  tprob = 0.0;
  nlistStartIterator (dancesel->base);
  while ((dkey = nlistIterateKey (dancesel->base)) >= 0) {
    abase = nlistGetDouble (dancesel->adjustBase, dkey);
    tval = abase / adjTotal;
    tprob += tval;
    nlistSetDouble (dancesel->danceProbTable, dkey, tprob);
  }

  /* ### copy the searchforpercentage routine from songsel and rework it */

  return 0;
}

/* internal routines */

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
