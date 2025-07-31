/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 *
 * 2024-4-1
 *   Re-written, but not tested.
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <memory.h>

#if __has_include (<MacTypes.h>)
# if defined (BDJ4_MEM_DEBUG)
#  undef BDJ4_MEM_DEBUG
# endif
#endif

#include "mdebug.h"
#include "volsink.h"
#include "volume.h"

enum {
  MAX_SINKS = 3,
};

static volsinklist_t gsinklist;
static int gvol [MAX_SINKS] = { 30, 20, 10 };
static bool ginit = false;

typedef struct {
  int       currsink;
  int       defsink;
  bool      changed;
} volnull_t;

static void volnullInitSinkList (void);

void
voliDesc (const char **ret, int max)
{
  int   c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "Null Audio";
  ret [c++] = NULL;
}

void
voliDisconnect (void)
{
  return;
}

void
voliCleanup (void **udata)
{
  volnull_t   *volnull;

  if (udata == NULL || *udata == NULL) {
    return;
  }

  volnull = *udata;
  mdfree (volnull);
  for (int i = 0; i < MAX_SINKS; ++i) {
    mdfree (gsinklist.sinklist [i].name);
    mdfree (gsinklist.sinklist [i].description);
  }
  mdfree (gsinklist.sinklist);
  ginit = false;
  return;
}

int
voliProcess (volaction_t action, const char *sinkname,
    int *vol, volsinklist_t *sinklist, void **udata)
{
  volnull_t     *volnull = NULL;
//  int           usersink = -1;
  int           defsink = -1;

  volnullInitSinkList ();

  if (action == VOL_HAVE_SINK_LIST) {
    return true;
  }

  if (udata == NULL) {
    return -1;
  }

  for (int i = 0; i < MAX_SINKS; ++i) {
    if (gsinklist.sinklist [i].defaultFlag) {
      defsink = i;
    }
    if (sinkname != NULL &&
        strcmp (gsinklist.sinklist [i].name, sinkname) == 0) {
//      usersink = i;
      break;
    }
  }

  if (*udata == NULL) {
    volnull = mdmalloc (sizeof (*volnull));
    *udata = volnull;
    volnull->currsink = 0;
    volnull->defsink = 0;
    volnull->changed = false;
    gsinklist.sinklist [volnull->defsink].defaultFlag = true;
  } else {
    volnull = *udata;
    if (defsink >= 0 && volnull->defsink != defsink) {
      gsinklist.sinklist [volnull->defsink].defaultFlag = false;
      volnull->defsink = defsink;
      gsinklist.sinklist [volnull->defsink].defaultFlag = true;
      volnull->changed = true;
    }
  }

  if (action == VOL_CHK_SINK) {
    bool    orig;

    orig = volnull->changed;
    volnull->changed = false;
    return orig;
  }

  if (action == VOL_GETSINKLIST) {
    sinklist->defname = mdstrdup (gsinklist.sinklist [volnull->defsink].name);
    sinklist->count = MAX_SINKS;
    sinklist->sinklist = mdmalloc (sinklist->count * sizeof (volsinkitem_t));
    for (int i = 0; i < MAX_SINKS; ++i) {
      sinklist->sinklist [i].name = mdstrdup (gsinklist.sinklist [i].name);
      sinklist->sinklist [i].description = mdstrdup (gsinklist.sinklist [i].description);
      sinklist->sinklist [i].defaultFlag = 0;
      sinklist->sinklist [i].idxNumber = i;
      sinklist->sinklist [i].nmlen = strlen (sinklist->sinklist [i].name);
    }
    sinklist->sinklist [volnull->defsink].defaultFlag = 1;
    return 0;
  }

  if (action == VOL_SET) {
    gvol [volnull->currsink] = *vol;
    if (gvol [volnull->currsink] < 0) { gvol [volnull->currsink] = 0; }
    if (gvol [volnull->currsink] > 100) { gvol [volnull->currsink] = 100; }
  }

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    *vol = gvol [volnull->currsink];

    return *vol;
  }

  return 0;
}

/* internal routines */

static void
volnullInitSinkList (void)
{
  if (ginit) {
    return;
  }

  gsinklist.sinklist = NULL;
  gsinklist.count = MAX_SINKS;
  gsinklist.sinklist = mdrealloc (gsinklist.sinklist,
      gsinklist.count * sizeof (volsinkitem_t));
  gsinklist.sinklist [0].name = mdstrdup ("no-volume");
  gsinklist.sinklist [0].description = mdstrdup ("No Volume");
  gsinklist.sinklist [1].name = mdstrdup ("silence");
  gsinklist.sinklist [1].description = mdstrdup ("Silence");
  gsinklist.sinklist [2].name = mdstrdup ("quiet");
  gsinklist.sinklist [2].description = mdstrdup ("Quiet");
  for (int i = 0; i < MAX_SINKS; ++i) {
    gsinklist.sinklist [i].defaultFlag = false;
    gsinklist.sinklist [i].idxNumber = i;
    gsinklist.sinklist [i].nmlen = 0;
    if (gsinklist.sinklist [i].name != NULL) {
      gsinklist.sinklist [i].nmlen = strlen (gsinklist.sinklist [i].name);
    }
  }
  ginit = true;
}
