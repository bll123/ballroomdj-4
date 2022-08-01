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
#include "osutils.h"
#include "playlist.h"
#include "queue.h"
#include "slist.h"

typedef struct {
  ilistidx_t    danceIdx;
} playedDance_t;

static bool     danceselProcessHistory (dancesel_t *dancesel,
                    ilistidx_t didx, ilistidx_t hdidx,
                    slist_t *tags, ssize_t histCount);
static bool     matchTag (slist_t *tags, slist_t *otags);

/* the countlist should contain a danceIdx/count pair */
dancesel_t *
danceselAlloc (nlist_t *countList)
{
  dancesel_t  *dancesel;
  ilistidx_t  didx;
  nlistidx_t  iteridx;


  if (countList == NULL) {
    return NULL;
  }

  logProcBegin (LOG_PROC, "danceselAlloc");
  dancesel = malloc (sizeof (dancesel_t));
  assert (dancesel != NULL);
  dancesel->dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancesel->autosel = bdjvarsdfGet (BDJVDF_AUTO_SEL);
  dancesel->base = nlistAlloc ("dancesel-base", LIST_ORDERED, NULL);
  dancesel->basetotal = 0.0;
  dancesel->distance = nlistAlloc ("dancesel-dist", LIST_ORDERED, NULL);
  dancesel->maxDistance = 0.0;
  dancesel->selectedCounts = nlistAlloc ("dancesel-sel-count", LIST_ORDERED, NULL);
  dancesel->playedDances = queueAlloc (free);
  dancesel->totalSelCount = 0.0;
  dancesel->adjustBase = NULL;
  dancesel->danceProbTable = NULL;
  dancesel->selCount = 0;

  /* autosel variables that will be used */
  dancesel->histDistance = autoselGetNum (dancesel->autosel, AUTOSEL_HIST_DISTANCE);
  dancesel->tagMatch = autoselGetDouble (dancesel->autosel, AUTOSEL_TAGMATCH);
  dancesel->tagAdjust = autoselGetDouble (dancesel->autosel, AUTOSEL_TAGADJUST);
  dancesel->logValue = autoselGetDouble (dancesel->autosel, AUTOSEL_LOG_VALUE);

  nlistStartIterator (countList, &iteridx);
  while ((didx = nlistIterateKey (countList, &iteridx)) >= 0) {
    long    count;

    count = nlistGetNum (countList, didx);
    if (count <= 0) {
      continue;
    }
    nlistSetNum (dancesel->base, didx, count);
    dancesel->basetotal += (double) count;
    logMsg (LOG_DBG, LOG_DANCESEL, "base: %d/%s: %d", didx,
        danceGetStr (dancesel->dances, didx, DANCE_DANCE), count);
    nlistSetNum (dancesel->selectedCounts, didx, 0);
  }

  nlistStartIterator (dancesel->base, &iteridx);
  while ((didx = nlistIterateKey (dancesel->base, &iteridx)) >= 0) {
    double  dcount, dtemp;

    dcount = (double) nlistGetNum (dancesel->base, didx);
    dtemp = floor (dancesel->basetotal / dcount);
    nlistSetDouble (dancesel->distance, didx, dtemp);
    logMsg (LOG_DBG, LOG_DANCESEL, "dist: %d/%s: %.2f", didx,
        danceGetStr (dancesel->dances, didx, DANCE_DANCE), dtemp);
    dancesel->maxDistance = (ssize_t)
        (dtemp > dancesel->maxDistance ? dtemp : dancesel->maxDistance);
  }
  dancesel->maxDistance = (ssize_t) round (dancesel->maxDistance);

  logProcEnd (LOG_PROC, "danceselAlloc", "");
  return dancesel;
}

void
danceselFree (dancesel_t *dancesel)
{
  logProcBegin (LOG_PROC, "danceselFree");
  if (dancesel != NULL) {
    if (dancesel->base != NULL) {
      nlistFree (dancesel->base);
    }
    if (dancesel->distance != NULL) {
      nlistFree (dancesel->distance);
    }
    if (dancesel->selectedCounts != NULL) {
      nlistFree (dancesel->selectedCounts);
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
  logProcEnd (LOG_PROC, "danceselFree", "");
}

void
danceselAddCount (dancesel_t *dancesel, ilistidx_t danceIdx)
{
  if (dancesel == NULL || dancesel->playedDances == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "danceselAddCount");

  nlistIncrement (dancesel->selectedCounts, danceIdx);
  ++dancesel->totalSelCount;
}

void
danceselAddPlayed (dancesel_t *dancesel, ilistidx_t danceIdx)
{
  playedDance_t *pd = NULL;

  if (dancesel == NULL || dancesel->playedDances == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "danceselAddPlayed");
  pd = malloc (sizeof (playedDance_t));
  pd->danceIdx = danceIdx;
  queuePushHead (dancesel->playedDances, pd);
  logProcEnd (LOG_PROC, "danceselAddPlayed", "");
}

ilistidx_t
danceselSelect (dancesel_t *dancesel, nlist_t *danceCounts,
    ssize_t priorCount, danceselHistory_t historyProc, void *userdata)
{
  ilistidx_t    didx;
  double        tbase;
  double        tprob;
  double        abase;
  nlistidx_t    iteridx;
  ssize_t       qiteridx;
  playedDance_t *pd;
  /* expected counts */
  double        expcount;
  double        dcount;
  /* current dance values */
  slist_t       *tags = NULL;
  double        speed;
  double        type;
  /* previous dance data */
  ilistidx_t    pddanceIdx = -1;
  slist_t       *pdtags = NULL;
  int           pdspeed = 0;
  int           pdtype = 0;
  /* history */
  dbidx_t       priorIdx;
  dbidx_t       histCount;
  ilistidx_t    hdidx;
  bool          hdone;
  /* probability calculation */
  double        adjTotal;
  double        tval;


  logProcBegin (LOG_PROC, "danceselSelect");
  if (dancesel->adjustBase != NULL) {
    nlistFree (dancesel->adjustBase);
  }
  if (dancesel->danceProbTable != NULL) {
    nlistFree (dancesel->danceProbTable);
  }
  dancesel->adjustBase = nlistAlloc ("dancesel-adjust-base", LIST_ORDERED, NULL);
  dancesel->danceProbTable = nlistAlloc ("dancesel-prob-table", LIST_ORDERED, NULL);

  /* data from the previous dance */
  if (priorCount > 0) {
    pddanceIdx = historyProc (userdata, priorCount - 1);
    logMsg (LOG_DBG, LOG_DANCESEL, "found previous dance %d/%s", pddanceIdx,
        danceGetStr (dancesel->dances, pddanceIdx, DANCE_DANCE));
    pdspeed = danceGetNum (dancesel->dances, pddanceIdx, DANCE_SPEED);
    pdtags = danceGetList (dancesel->dances, pddanceIdx, DANCE_TAGS);
    pdtype = danceGetNum (dancesel->dances, pddanceIdx, DANCE_TYPE);
  } else {
    logMsg (LOG_DBG, LOG_DANCESEL, "no previous dance");
  }

  nlistStartIterator (dancesel->base, &iteridx);
  while ((didx = nlistIterateKey (dancesel->base, &iteridx)) >= 0) {
    tbase = (double) nlistGetNum (dancesel->base, didx);
    tprob = tbase / dancesel->basetotal;
    abase = tbase;
    logMsg (LOG_DBG, LOG_DANCESEL, "didx:%d/%s base:%.2f tprob:%.6f",
        didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE),
        tbase, tprob);

    /* are any dances of this type still available ? */
    dcount = (double) nlistGetNum (danceCounts, didx);
    if (dcount <= 0) {
      abase = 0.0;
      nlistSetDouble (dancesel->adjustBase, didx, abase);
      logMsg (LOG_DBG, LOG_DANCESEL, "  no more %d/%s left", didx,
          danceGetStr (dancesel->dances, didx, DANCE_DANCE));
      /* no need to do the other checks */
      continue;
    }

    expcount = fmax (1.0, round (tprob * dancesel->totalSelCount + 0.5));
    dcount = (double) nlistGetNum (dancesel->selectedCounts, didx);
    speed = danceGetNum (dancesel->dances, didx, DANCE_SPEED);

    /* expected count high ( / 800 ) */
    if (dcount > expcount) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_LIMIT);
      logMsg (LOG_DBG, LOG_DANCESEL, "  dcount %.6f > expcount %.6f / %.6f", dcount, expcount, abase);
    }

    /* expected count low ( / 0.5 ) */
    if (dcount < expcount - 1) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_LOW);
      logMsg (LOG_DBG, LOG_DANCESEL, "  dcount %.6f < expcount %.6f / %.6f", dcount, expcount - 1, abase);
    }

    /* if this selection is at the beginning of the playlist */
    if (dancesel->selCount < autoselGetNum (dancesel->autosel, AUTOSEL_BEG_COUNT)) {
      /* if this dance is a fast dance ( / 1000) */
      if (speed == DANCE_SPEED_FAST) {
        abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_BEG_FAST);
        logMsg (LOG_DBG, LOG_DANCESEL, "  fast / begin of playlist / %.6f", abase);
      }
    }

    /* if this dance and the previous dance were both fast ( / 1000) */
    if (priorCount > 0 && pddanceIdx >= 0 &&
        pdspeed == speed && speed == DANCE_SPEED_FAST) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_BOTHFAST);
      logMsg (LOG_DBG, LOG_DANCESEL, "  speed is fast and same as previous / %.6f", abase);
    }

    /* if there is a tag match between the previous dance and this one */
    /* ( / 600 ) */
    tags = danceGetList (dancesel->dances, didx, DANCE_TAGS);
    if (priorCount > 0 && pddanceIdx >= 0 && matchTag (tags, pdtags)) {
      abase = abase / dancesel->tagMatch;
      logMsg (LOG_DBG, LOG_DANCESEL, "  matched tags with previous / %.6f", abase);
    }

    /* if this dance and the previous dance have matching types ( / 600 ) */
    type = danceGetNum (dancesel->dances, didx, DANCE_TYPE);
    if (priorCount > 0 && pddanceIdx >= 0 && pdtype == type) {
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_TYPE_MATCH);
      logMsg (LOG_DBG, LOG_DANCESEL, "  matched type with previous / %.6f", abase);
    }

    nlistSetDouble (dancesel->adjustBase, didx, abase);

    if (priorCount > 0) {
      /* now go back through the history */
      priorIdx = priorCount - 1;

      queueStartIterator (dancesel->playedDances, &qiteridx);

      histCount = 1;
      hdone = false;
      while (! hdone &&
          histCount < dancesel->maxDistance &&
          (hdidx = historyProc (userdata, priorIdx)) >= 0) {
        hdone = danceselProcessHistory (dancesel, didx, hdidx,
            tags, histCount);
        ++histCount;
        --priorIdx;
      } /* for each of the history */

      logMsg (LOG_DBG, LOG_DANCESEL, "  played dance (%d)", histCount);
      while (! hdone &&
          histCount < dancesel->maxDistance &&
          (pd = queueIterateData (dancesel->playedDances, &qiteridx)) != NULL) {
        hdidx = pd->danceIdx;
        hdone = danceselProcessHistory (dancesel, didx, hdidx,
            tags, histCount);
        ++histCount;
      } /* for each of the played dances */
    } /* if there is history to be checked */
  } /* for each dance in the base list */

  /* get the totals for the adjusted base values */
  adjTotal = 0.0;
  nlistStartIterator (dancesel->adjustBase, &iteridx);
  while ((didx = nlistIterateKey (dancesel->adjustBase, &iteridx)) >= 0) {
    abase = nlistGetDouble (dancesel->adjustBase, didx);
    adjTotal += abase;
  }

  /* create a probability table of running totals */
  tprob = 0.0;
  nlistStartIterator (dancesel->adjustBase, &iteridx);
  while ((didx = nlistIterateKey (dancesel->adjustBase, &iteridx)) >= 0) {
    abase = nlistGetDouble (dancesel->adjustBase, didx);
    tval = abase / adjTotal;
    tprob += tval;
    logMsg (LOG_DBG, LOG_DANCESEL, "final prob: %d/%s: %.6f",
        didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE), tprob);
    nlistSetDouble (dancesel->danceProbTable, didx, tprob);
  }

  tval = dRandom ();
  didx = nlistSearchProbTable (dancesel->danceProbTable, tval);
  logMsg (LOG_DBG, LOG_DANCESEL, "select %d/%s",
        didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE));

  logProcEnd (LOG_PROC, "danceselSelect", "");
  return didx;
}

/* internal routines */

static bool
danceselProcessHistory (dancesel_t *dancesel, ilistidx_t didx,
    ilistidx_t hdidx, slist_t *tags, ssize_t histCount)
{
  /* working with the history (played dances) */
  slist_t       *htags = NULL;
  double        dist;
  double        tval;
  double        findex;
  double        abase;

  abase = nlistGetDouble (dancesel->adjustBase, didx);

  logProcBegin (LOG_PROC, "danceselProcessHistory");
  logMsg (LOG_DBG, LOG_DANCESEL, "  history (%d) didx:%d hdidx:%d/%s",
      histCount, didx,
      hdidx, danceGetStr (dancesel->dances, hdidx, DANCE_DANCE));

  /* the tags of the first few dances in the history are checked */
  /* the previous dance's tags have already been adjusted */
  if (histCount > 1 && histCount <= dancesel->histDistance) {
    htags = danceGetList (dancesel->dances, hdidx, DANCE_TAGS);
    if (matchTag (tags, htags)) {
      tval = pow (dancesel->tagAdjust, (double) histCount);
      abase = abase / dancesel->tagMatch * tval;
      nlistSetDouble (dancesel->adjustBase, didx, abase);
      logMsg (LOG_DBG, LOG_DANCESEL, "    matched history tags (%d) / %.6f", histCount, abase);
    }
  }

  if (hdidx != didx) {
    logProcEnd (LOG_PROC, "danceselProcessHistory", "no-match");
    return false;
  }

  /* found the matching dance */
  logMsg (LOG_DBG, LOG_DANCESEL, "    matched dance (%d) hdidx:%d", histCount, hdidx);

  dist = nlistGetDouble (dancesel->distance, hdidx);
  tval = (double) (histCount - 1);

  /* if the dance hasn't appeared within 'dist', then no adjustment */
  /* is needed */
  if (dist < tval || abase == 0.0) {
    logMsg (LOG_DBG, LOG_DANCESEL, "    dist %.2f < tval %.2f", dist, tval);
    if (histCount > dancesel->histDistance) {
      /* no more history needs to be checked */
      logProcEnd (LOG_PROC, "danceselProcessHistory", "no-adjust-fin");
      return true;
    }
    logProcEnd (LOG_PROC, "danceselProcessHistory", "no-adjust-not-fin");
    return false;
  }

  /* logarithmic */
  findex = fmax (0.0, fmin (1.0, tval / dist));
  findex = 1.0 - (pow (0.1, (1.0 - findex) * dancesel->logValue));
  logMsg (LOG_DBG, LOG_DANCESEL, "    dist adjust: tval:%.6f dist:%.6f findex:%.6f", tval, dist, findex);

  abase = fmax (0.0, abase - (findex * abase));
  logMsg (LOG_DBG, LOG_DANCESEL, "    adjust due to distance / %.6f", abase);
  nlistSetDouble (dancesel->adjustBase, hdidx, abase);

  /* having reached this point, the dance has been found  */
  /* the matching dance history processing is finished    */
  /* the tag history check also needs to be finished      */
  if (histCount > dancesel->histDistance) {
    logProcEnd (LOG_PROC, "danceselProcessHistory", "adjust-fin");
    return true;
  }

  logProcEnd (LOG_PROC, "danceselProcessHistory", "adjust-not-fin");
  return false;
}


static bool
matchTag (slist_t *tags, slist_t *otags)
{
  char        *ttag;
  char        *otag;
  slistidx_t  titeridx;
  slistidx_t  oiteridx;

  if (tags != NULL && otags != NULL) {
    slistStartIterator (tags, &titeridx);
    while ((ttag = slistIterateKey (tags, &titeridx)) != NULL) {
      slistStartIterator (otags, &oiteridx);
      while ((otag = slistIterateKey (otags, &oiteridx)) != NULL) {
        if (strcmp (ttag, otag) == 0) {
          return true;
        }
      } /* for each tag for the previous dance */
    } /* for each tag for this dance */
  } /* if both dances have tags */
  return false;
}

