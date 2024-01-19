/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include "bdj4.h"
#include "autosel.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "dancesel.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "osrandom.h"
#include "playlist.h"
#include "queue.h"
#include "slist.h"

typedef struct {
  ilistidx_t    danceIdx;
} playedDance_t;

enum {
  DANCESEL_DEBUG = 0,
};

typedef struct dancesel {
  int           method;
  dance_t       *dances;
  autosel_t     *autosel;
  danceselQueueLookup_t queueLookupProc;
  void          *userdata;
  /* all methods */
  double        basetotal;
  nlist_t       *base;
  nlist_t       *adjustBase;
  nlist_t       *danceProbTable;
  ssize_t       selCount;
  /* windowed probability tables */
  nlist_t       *winsize;
  nlist_t       *wdecrement;
  nlist_t       *winEarlySel;
  /* played */
  queue_t       *playedDances;
  /* autosel variables that will be used */
  nlistidx_t    histDistance;
  double        prevTagMatch;
  double        priorExp;
  double        tagMatch;
  /* windowed */
  double        windowedDiffA;
  double        windowedDiffB;
  double        windowedDiffC;
} dancesel_t;

static void   danceselPlayedFree (void *data);
static bool   danceselProcessPrior (dancesel_t *dancesel,
                    ilistidx_t didx, ilistidx_t priordidx, ilistidx_t dist,
                    slist_t *tags, bool match);
static bool   danceselMatchTag (slist_t *tags, slist_t *otags);
static bool   danceselGetPriorInfo (dancesel_t *dancesel,
                    ilistidx_t queueCount, ilistidx_t prioridx,
                    ilistidx_t *pddanceIdx);

/* windowed */
static void   danceselInitWindowSizes (dancesel_t *dancesel);
static void   danceselInitDecrement (dancesel_t *dancesel);

/* the countlist should contain a danceIdx/count pair */
dancesel_t *
danceselAlloc (nlist_t *countList,
    danceselQueueLookup_t queueLookupProc, void *userdata)
{
  dancesel_t  *dancesel;
  ilistidx_t  didx;
  nlistidx_t  iteridx;


  if (countList == NULL) {
    return NULL;
  }

  logProcBegin (LOG_PROC, "danceselAlloc");
  dancesel = mdmalloc (sizeof (dancesel_t));

  dancesel->method = bdjoptGetNum (OPT_G_DANCESEL_METHOD);
  logMsg (LOG_DBG, LOG_INFO, "dancesel-method: %d", dancesel->method);
  dancesel->dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancesel->autosel = bdjvarsdfGet (BDJVDF_AUTO_SEL);

  dancesel->base = nlistAlloc ("dancesel-base", LIST_ORDERED, NULL);
  dancesel->basetotal = 0.0;
  dancesel->adjustBase = NULL;
  dancesel->danceProbTable = NULL;
  dancesel->queueLookupProc = queueLookupProc;
  dancesel->userdata = userdata;

  /* windowed */
  dancesel->winsize = nlistAlloc ("dancesel-winsize", LIST_ORDERED, NULL);
  dancesel->wdecrement = nlistAlloc ("dancesel-w-decrement", LIST_ORDERED, NULL);
  dancesel->winEarlySel = nlistAlloc ("dancesel-w-early-sel", LIST_ORDERED, NULL);

  /* selected */
  dancesel->selCount = 0;

  /* played */
  dancesel->playedDances = queueAlloc ("played-dances", danceselPlayedFree);

  /* autosel variables that will be used */
  dancesel->histDistance = autoselGetNum (dancesel->autosel, AUTOSEL_HIST_DISTANCE);
  dancesel->prevTagMatch = autoselGetDouble (dancesel->autosel, AUTOSEL_PREV_TAGMATCH);
  dancesel->tagMatch = autoselGetDouble (dancesel->autosel, AUTOSEL_TAGMATCH);
  dancesel->priorExp = autoselGetDouble (dancesel->autosel, AUTOSEL_PRIOR_EXP);
  /* windowed */
  dancesel->windowedDiffA = autoselGetDouble (dancesel->autosel, AUTOSEL_WINDOWED_DIFF_A);
  dancesel->windowedDiffB = autoselGetDouble (dancesel->autosel, AUTOSEL_WINDOWED_DIFF_B);
  dancesel->windowedDiffC = autoselGetDouble (dancesel->autosel, AUTOSEL_WINDOWED_DIFF_C);

  logMsg (LOG_DBG, LOG_DANCESEL, "countlist: %d", nlistGetCount (countList));
  if (DANCESEL_DEBUG) {
    fprintf (stderr, "countlist: %d\n", nlistGetCount (countList));
  }

  nlistStartIterator (countList, &iteridx);
  while ((didx = nlistIterateKey (countList, &iteridx)) >= 0) {
    long    count;

    count = nlistGetNum (countList, didx);
    if (count <= 0) {
      continue;
    }

    nlistSetNum (dancesel->base, didx, count);
    dancesel->basetotal += (double) count;
    logMsg (LOG_DBG, LOG_DANCESEL, "base: %d/%s: %ld", didx,
        danceGetStr (dancesel->dances, didx, DANCE_DANCE), count);
    if (DANCESEL_DEBUG) {
      fprintf (stderr, "base: %d/%s: %ld\n", didx,
          danceGetStr (dancesel->dances, didx, DANCE_DANCE), count);
    }
  }

  if (dancesel->method == DANCESEL_METHOD_WINDOWED) {
    danceselInitWindowSizes (dancesel);
    danceselInitDecrement (dancesel);
  } /* method == windowed */

  logProcEnd (LOG_PROC, "danceselAlloc", "");
  return dancesel;
}

void
danceselFree (dancesel_t *dancesel)
{
  logProcBegin (LOG_PROC, "danceselFree");
  if (dancesel != NULL) {
    nlistFree (dancesel->base);
    nlistFree (dancesel->adjustBase);
    nlistFree (dancesel->danceProbTable);
    queueFree (dancesel->playedDances);
    /* windowed */
    nlistFree (dancesel->winsize);
    nlistFree (dancesel->wdecrement);
    nlistFree (dancesel->winEarlySel);
    mdfree (dancesel);
  }
  logProcEnd (LOG_PROC, "danceselFree", "");
}

/* used by main for mix */
void
danceselDecrementBase (dancesel_t *dancesel, ilistidx_t danceIdx)
{
  if (danceIdx < 0 || dancesel == NULL || dancesel->playedDances == NULL) {
    return;
  }

  /* all methods */
  nlistDecrement (dancesel->base, danceIdx);
  dancesel->basetotal -= 1.0;
  logMsg (LOG_DBG, LOG_DANCESEL, "decrement %d/%s now %ld total %.0f",
      danceIdx, danceGetStr (dancesel->dances, danceIdx, DANCE_DANCE),
      (long) nlistGetNum (dancesel->base, danceIdx), dancesel->basetotal);

  /* base and basetotal are already decremented */
  if (dancesel->method == DANCESEL_METHOD_WINDOWED) {
    danceselInitWindowSizes (dancesel);
  } /* method == windowed */
}

void
danceselAddCount (dancesel_t *dancesel, ilistidx_t danceIdx)
{
  if (dancesel == NULL) {
    return;
  }

  ++dancesel->selCount;

  if (dancesel->method == DANCESEL_METHOD_WINDOWED) {
    nlistidx_t    iteridx;
    ilistidx_t    didx;

    /* the selected dance decrement is increased */
    /* any dance with a positive decrement is increased */

    nlistStartIterator (dancesel->base, &iteridx);
    while ((didx = nlistIterateKey (dancesel->base, &iteridx)) >= 0) {
      double    dval;

      dval = nlistGetDouble (dancesel->wdecrement, didx);
      if (dval > 0.0 || didx == danceIdx) {
        dval += 1.0;
        nlistSetDouble (dancesel->wdecrement, didx, dval);
        logMsg (LOG_DBG, LOG_DANCESEL, "win: decrement %d/%s %.2f",
            didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE),
            nlistGetDouble (dancesel->wdecrement, didx));
        if (DANCESEL_DEBUG) {
          fprintf (stderr, "win: decrement %d/%s %.2f\n",
              didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE),
              nlistGetDouble (dancesel->wdecrement, didx));
        }
      }
    }
  } /* method == windowed */
}

/* played history queue */
/* this is only relevant if there is nothing queued by the caller */
void
danceselAddPlayed (dancesel_t *dancesel, ilistidx_t danceIdx)
{
  playedDance_t *pd = NULL;

  if (dancesel == NULL || dancesel->playedDances == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "danceselAddPlayed");
  pd = mdmalloc (sizeof (playedDance_t));
  pd->danceIdx = danceIdx;
  queuePushHead (dancesel->playedDances, pd);
  logProcEnd (LOG_PROC, "danceselAddPlayed", "");
}

/* after selecting a dance, call danceselAddCount() if the dance will */
/* be used as a selection */
ilistidx_t
danceselSelect (dancesel_t *dancesel, ilistidx_t queueCount)
{
  ilistidx_t    didx;
  double        tbase;
  double        tprob;
  double        abase;
  nlistidx_t    iteridx;
  int           countAvailable;
  int           countTries;
  /* current dance values */
  slist_t       *tags = NULL;
  int           speed;
  int           type;
  /* previous dance data */
  ilistidx_t    pddanceIdx;
  slist_t       *pdtags = NULL;
  int           pdspeed = 0;
  int           pdtype = 0;
  /* queued dances */
  ilistidx_t    queueIdx;
  ilistidx_t    priordidx;        // prior dance index
  ilistidx_t    queueDist;
  ilistidx_t    chkdist;
  bool          priorLookupDone;
  /* probability calculation */
  double        adjTotal;
  double        tval;


  logProcBegin (LOG_PROC, "danceselSelect");
  nlistFree (dancesel->adjustBase);
  nlistFree (dancesel->danceProbTable);
  dancesel->adjustBase = nlistAlloc ("dancesel-adjust-base", LIST_ORDERED, NULL);
  dancesel->danceProbTable = nlistAlloc ("dancesel-prob-table", LIST_ORDERED, NULL);

  /* data from the previous dance */

  danceselGetPriorInfo (dancesel, queueCount, queueCount - 1, &pddanceIdx);

  if (pddanceIdx >= 0) {
    logMsg (LOG_DBG, LOG_DANCESEL, "found previous dance %d/%s", pddanceIdx,
        danceGetStr (dancesel->dances, pddanceIdx, DANCE_DANCE));
    if (DANCESEL_DEBUG) {
      fprintf (stderr, "  get previous dance: %d/%s\n", pddanceIdx,
          danceGetStr (dancesel->dances, pddanceIdx, DANCE_DANCE));
    }
    pdspeed = danceGetNum (dancesel->dances, pddanceIdx, DANCE_SPEED);
    pdtags = danceGetList (dancesel->dances, pddanceIdx, DANCE_TAGS);
    pdtype = danceGetNum (dancesel->dances, pddanceIdx, DANCE_TYPE);
  } else {
    logMsg (LOG_DBG, LOG_DANCESEL, "  no previous dance");
  }

  countAvailable = 0;
  countTries = 0;
  while (countAvailable == 0) {
    nlistStartIterator (dancesel->base, &iteridx);
    while ((didx = nlistIterateKey (dancesel->base, &iteridx)) >= 0) {

      /* if re-trying, give all dances a small chance, and */
      /* raise the chance as the number of tries goes up. */
      /* the prevents starvation when the possibilities become low */
      tbase = countTries * 0.1;

      /* at this time, only the 'windowed' method is implemented. */
      /* there was a (more complicated) 'expected-count' method, */
      /* but it has been removed. */

      if (dancesel->method == DANCESEL_METHOD_WINDOWED) {
        double    twinsz;
        double    diff;
        double    dec;

        twinsz = nlistGetDouble (dancesel->winsize, didx);
        if (twinsz <= 0.0) {
          continue;
        }

        dec = nlistGetDouble (dancesel->wdecrement, didx);
        diff = twinsz - dec;

        nlistSetNum (dancesel->winEarlySel, didx, false);

        /* base value for the dance */
        /* determine which dances are allowed to be selected */
        /* always 1.0 unless the dance is outside the window */
        /* if outside the window, use the diff* values to give */
        /* the dance a small chance of being selected */

        /* wsz   dec   diff   tbase */
        /* 3.4 - 0.0 :  3.4 : 1.0 */
        /* 3.4 - 1.0 :  2.4 : 0.1 */
        /* 3.4 - 2.0 :  1.4 : 0.25 */
        /* 3.4 - 3.0 :  0.4 : 0.5 */
        /* 3.4 - 4.0 : -0.4 : 1.0, reset decrement */
        if (diff <= 0.0) {
          tbase = 1.0;
          nlistSetDouble (dancesel->wdecrement, didx, 0.0);
          nlistSetNum (dancesel->winEarlySel, didx, false);
        } else if (diff >= twinsz) {
          /* decrement is zero */
          tbase = 1.0;
        } else {
          if (diff > 0.0) {
            tbase = countTries * 0.1;
            if (! nlistGetNum (dancesel->winEarlySel, didx)) {
              /* these allow the dance to be selected a bit early in the window */
              /* this helps prevent situations where no dance can be selected */
              /* in these cases, if the dance is selected, make sure the dance */
              /* is not re-selected until the decrement is reset */
              if (diff <= 3.0) {
                tbase = dancesel->windowedDiffC;
                nlistSetNum (dancesel->winEarlySel, didx, true);
              }
              if (diff <= 2.0) {
                tbase = dancesel->windowedDiffB;
                nlistSetNum (dancesel->winEarlySel, didx, true);
              }
            }
            /* on the edge between windows, 50% chance */
            if (diff <= 1.0) {
              tbase = dancesel->windowedDiffA;
            }
          }
        }

        logMsg (LOG_DBG, LOG_DANCESEL, "win:  didx:%d/%s",
            didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE));
        logMsg (LOG_DBG, LOG_DANCESEL, "win:    winsz:%.2f decr:%.2f diff:%.2f",
            twinsz, dec, diff);
        logMsg (LOG_DBG, LOG_DANCESEL, "win:    base-prob:%.2f", tbase);
        if (DANCESEL_DEBUG) {
          fprintf (stderr, "win:  didx:%d/%s\n",
              didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE));
          fprintf (stderr, "win:    winsz:%.2f decr:%.2f diff:%.2f\n",
              twinsz, dec, diff);
          fprintf (stderr, "win:    base-prob:%.2f\n", tbase);
        }

        if (tbase == 0.0) {
          continue;
        }
      } /* method = windowed */

      abase = tbase;
      logMsg (LOG_DBG, LOG_DANCESEL, "  didx:%d/%s base:%.0f",
          didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE), tbase);
      if (DANCESEL_DEBUG) {
        fprintf (stderr, "  didx:%d/%s base:%.0f\n",
          didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE), tbase);
      }

      speed = danceGetNum (dancesel->dances, didx, DANCE_SPEED);

      nlistSetDouble (dancesel->adjustBase, didx, abase);

      /* if this selection is at the beginning of the playlist */

      if (dancesel->selCount < autoselGetNum (dancesel->autosel, AUTOSEL_BEG_COUNT)) {
        /* if this dance is a fast dance ( / 1000) */
        if (speed == DANCE_SPEED_FAST) {
          abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_BEG_FAST);
          logMsg (LOG_DBG, LOG_DANCESEL, "   fast / begin of playlist: abase: %.6f", abase);
          if (DANCESEL_DEBUG) {
            fprintf (stderr, "   fast / begin of playlist: abase: %.6f\n", abase);
            fprintf (stderr, "     selcount: %ld\n", (long) dancesel->selCount);
          }
        }
      }

      /* if this dance and the previous dance were both fast ( / 1000) */

      if (speed == DANCE_SPEED_FAST &&
          pddanceIdx >= 0 && pdspeed == speed) {
        abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_FAST_BOTH);
        logMsg (LOG_DBG, LOG_DANCESEL, "   speed is fast and same as previous: abase: %.6f", abase);
        if (DANCESEL_DEBUG) {
          fprintf (stderr, "   speed is fast and same as previous: abase: %.6f\n", abase);
        }
      }

      /* if this dance and the previous dance have matching types ( / 600 ) */

      type = danceGetNum (dancesel->dances, didx, DANCE_TYPE);
      if (pddanceIdx >= 0 && pdtype == type) {
        abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_TYPE_MATCH);
        logMsg (LOG_DBG, LOG_DANCESEL, "   matched type with previous: abase: %.6f", abase);
        if (DANCESEL_DEBUG) {
          fprintf (stderr, "   matched type with previous: abase: %.6f\n", abase);
        }
      }

      /* if there is a tag match between the previous dance and this one */
      /* ( / 600 ) */

      tags = danceGetList (dancesel->dances, didx, DANCE_TAGS);
      if (pddanceIdx >= 0 && danceselMatchTag (tags, pdtags)) {
        abase = abase / dancesel->prevTagMatch;
        logMsg (LOG_DBG, LOG_DANCESEL, "   matched tags with previous: abase: %.6f", abase);
        if (DANCESEL_DEBUG) {
          fprintf (stderr, "   matched tags with previous: abase: %.6f\n", abase);
        }
      }

      /* process prior checks, save abase */

      nlistSetDouble (dancesel->adjustBase, didx, abase);

      /* the previous dance (dist == 1) has already been checked */
      queueDist = 2;
      queueIdx = queueCount - queueDist;
      priorLookupDone = false;
      priordidx = -1;
      chkdist = dancesel->histDistance;

      while (! priorLookupDone && queueDist < chkdist) {
        bool  match = false;

        priorLookupDone = danceselGetPriorInfo (dancesel, queueCount, queueIdx, &priordidx);
        /* priordidx will be -1 if there is no prior dance*/
        if (! priorLookupDone && priordidx != -1) {
          logMsg (LOG_DBG, LOG_DANCESEL, "   prior dist:%d", queueDist);
          if (DANCESEL_DEBUG) {
            fprintf (stderr, "   prior dist:%d\n", queueDist);
          }
          /* process prior is only done for matching dance indexes */
          match = danceselProcessPrior (dancesel, didx, priordidx, queueDist, tags, match);
        }
        --queueIdx;
        ++queueDist;
      } /* for prior queue/played entries */
    } /* for each dance in the base list */

    countAvailable = nlistGetCount (dancesel->adjustBase);
    countTries += 1;
    if (countAvailable == 0) {
      /* the windowed method can end up with no selections available, */
      /* as the window for every dance can be active, especially near */
      /* the 'basetotal'. */
      logMsg (LOG_DBG, LOG_DANCESEL, "--- no available selections");
      if (DANCESEL_DEBUG) {
        fprintf (stderr, "--- no available selections\n");
      }
    }
  } /* outside loop to make sure something is available to be selected */

  /* get the total for the adjusted base values */

  adjTotal = 0.0;
  nlistStartIterator (dancesel->adjustBase, &iteridx);
  while ((didx = nlistIterateKey (dancesel->adjustBase, &iteridx)) >= 0) {
    abase = nlistGetDouble (dancesel->adjustBase, didx);
    if (abase > 0.0) {
      logMsg (LOG_DBG, LOG_DANCESEL, "    pre-final:%d/%s %.6f", didx,
            danceGetStr (dancesel->dances, didx, DANCE_DANCE), abase);
      if (DANCESEL_DEBUG) {
        fprintf (stderr, "    pre-final:%d/%s %.6f\n", didx,
            danceGetStr (dancesel->dances, didx, DANCE_DANCE), abase);
      }
    }
    adjTotal += abase;
  }

  /* create a probability table of running totals */

  tprob = 0.0;
  nlistStartIterator (dancesel->adjustBase, &iteridx);
  while ((didx = nlistIterateKey (dancesel->adjustBase, &iteridx)) >= 0) {
    abase = nlistGetDouble (dancesel->adjustBase, didx);
    tval = 0.0;
    if (adjTotal > 0.0) {
      tval = abase / adjTotal;
    }
    tprob += tval;
    logMsg (LOG_DBG, LOG_DANCESEL, "  final prob: %d/%s: %.6f",
        didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE), tprob);
    if (DANCESEL_DEBUG) {
      fprintf (stderr, "     final prob: %d/%s: %.6f\n",
          didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE), tprob);
    }
    nlistSetDouble (dancesel->danceProbTable, didx, tprob);
  }

  /* and pick a dance */

  tval = dRandom ();
  didx = nlistSearchProbTable (dancesel->danceProbTable, tval);
  logMsg (LOG_DBG, LOG_BASIC, "== select %.6f %d/%s",
        tval, didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE));
  if (DANCESEL_DEBUG) {
    fprintf (stderr, "== select %.6f %d/%s\n",
        tval, didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE));
  }
  if (dancesel->method == DANCESEL_METHOD_WINDOWED) {
    /* if the dance was an early selection, reset the decrement */
    if (nlistGetNum (dancesel->winEarlySel, didx)) {
      nlistSetDouble (dancesel->wdecrement, didx, 0.0);
      nlistSetNum (dancesel->winEarlySel, didx, false);
    }
  } /* method = windowed */

  logProcEnd (LOG_PROC, "danceselSelect", "");
  return didx;
}

/* internal routines */

static void
danceselPlayedFree (void *data)
{
  playedDance_t *pd = data;

  dataFree (pd);
}

static bool
danceselProcessPrior (dancesel_t *dancesel, ilistidx_t didx,
    ilistidx_t priordidx, ilistidx_t priordist, slist_t *tags, bool match)
{
  slist_t       *priortags = NULL;
  double        abase;
  int           matchrc = false;

  logProcBegin (LOG_PROC, "danceselProcessPrior");

  /* only the first hist-distance songs are checked */

  if (priordist > dancesel->histDistance) {
    logProcEnd (LOG_PROC, "danceselProcessPrior", "past-distance");
    return false;
  }

  abase = nlistGetDouble (dancesel->adjustBase, didx);
  if (abase == 0.0) {
    logProcEnd (LOG_PROC, "danceselProcessPrior", "at-zero");
    return false;
  }

  /* the previous dance's tags have already been adjusted */

  if (priordist > 0) {
    logMsg (LOG_DBG, LOG_DANCESEL, "     process prior didx:%d/%s prior:%d/%s",
        didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE),
        priordidx, danceGetStr (dancesel->dances, priordidx, DANCE_DANCE));
  }

  /* check the speed of the previous dance if the priordist == 2 */

  if (! match && priordist == 2) {
    int     pspeed;
    int     speed;

    pspeed = danceGetNum (dancesel->dances, priordidx, DANCE_SPEED);
    speed = danceGetNum (dancesel->dances, didx, DANCE_SPEED);
    if (speed == DANCE_SPEED_FAST && pspeed == speed) {
      double  tmp;

      tmp = autoselGetDouble (dancesel->autosel, AUTOSEL_FAST_PRIOR);
      logMsg (LOG_DBG, LOG_DANCESEL, "     fastmatch adj: %.6f old-abase: %.6f", tmp, abase);
      abase = abase / autoselGetDouble (dancesel->autosel, AUTOSEL_FAST_PRIOR);
      logMsg (LOG_DBG, LOG_DANCESEL, "     fastmatch abase: %.6f", abase);
      if (DANCESEL_DEBUG) {
        fprintf (stderr, "     fastmatch abase: %.6f", abase);
      }
      matchrc = true;
    }
  }

  /* the tags of the first few dances in the history are checked */
  /* when matching, don't process another match if one has already been found */

  if (! match && priordist > 0 && priordist < dancesel->histDistance) {
    double    tmp;

    priortags = danceGetList (dancesel->dances, priordidx, DANCE_TAGS);

    if (danceselMatchTag (tags, priortags)) {
      /* further distance, smaller value, minimum no change */
      tmp = fmax (1.0, (dancesel->tagMatch / pow (priordist, dancesel->priorExp)));
      logMsg (LOG_DBG, LOG_DANCESEL, "     tagmatch adj: %.6f old-abase: %.6f", tmp, abase);
      abase = abase / tmp;
      nlistSetDouble (dancesel->adjustBase, didx, abase);
      logMsg (LOG_DBG, LOG_DANCESEL, "     tagmatch tags: abase: %.6f", abase);
      if (DANCESEL_DEBUG) {
        fprintf (stderr, "      tagmatch adj: %.6f abase: %.6f\n", tmp, abase);
      }
      matchrc = true;
    }
  }

  nlistSetDouble (dancesel->adjustBase, didx, abase);

  logProcEnd (LOG_PROC, "danceselProcessPrior", "");
  return matchrc;
}


static bool
danceselMatchTag (slist_t *tags, slist_t *otags)
{
  const char  *ttag;
  const char  *otag;
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

static bool
danceselGetPriorInfo (dancesel_t *dancesel, ilistidx_t queueCount,
    ilistidx_t queueIdx, ilistidx_t *pddanceIdx)
{
  ilistidx_t    didx = -1;
  playedDance_t *pd = NULL;
  bool          rc = false;

  /* first try the lookup in the queue */

  if (dancesel->queueLookupProc != NULL) {
    didx = dancesel->queueLookupProc (dancesel->userdata, queueIdx);
  }

  /* during a 'mix', there is an empty-head in position 0 */

  if (didx < 0 && queueIdx != 0) {
    ilistidx_t    count;

    queueIdx = queueIdx * -1 - 1;
    count = queueGetCount (dancesel->playedDances);
    if (count <= 0 || count < queueIdx) {
      /* no more data */
      rc = true;
    } else {
      pd = queueGetByIdx (dancesel->playedDances, queueIdx);
      if (pd != NULL) {
        didx = pd->danceIdx;
      }
    }
  }

  *pddanceIdx = didx;
  return rc;
}

/* windowed */
static void
danceselInitWindowSizes (dancesel_t *dancesel)
{
  nlistidx_t    iteridx;
  ilistidx_t    didx;

  nlistStartIterator (dancesel->base, &iteridx);
  while ((didx = nlistIterateKey (dancesel->base, &iteridx)) >= 0) {
    long    count;
    double  dval;

    count = nlistGetNum (dancesel->base, didx);

    /* windowed */
    /* this sets the window size for the dance */
    if (count == 0) {
      dval = 0.0;
    } else {
      dval = (double) dancesel->basetotal / (double) count;
      /* reduce the window size slightly. */
      /* this helps prevent starvation near the 'basetotal' */
      dval -= 0.3;
    }
    nlistSetDouble (dancesel->winsize, didx, dval);
    if (dval > 0.0) {
      logMsg (LOG_DBG, LOG_DANCESEL, "win: winsize: %d/%s %.2f",
          didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE),
          dval);
      if (DANCESEL_DEBUG) {
        fprintf (stderr, "win: winsize: %d/%s %.2f\n",
            didx, danceGetStr (dancesel->dances, didx, DANCE_DANCE),
            dval);
      }
    }
  }
}

/* windowed */
static void
danceselInitDecrement (dancesel_t *dancesel)
{
  nlistidx_t    iteridx;
  ilistidx_t    didx;

  nlistStartIterator (dancesel->base, &iteridx);
  while ((didx = nlistIterateKey (dancesel->base, &iteridx)) >= 0) {
    nlistSetDouble (dancesel->wdecrement, didx, 0.0);
    nlistSetNum (dancesel->winEarlySel, didx, false);
  }
}
