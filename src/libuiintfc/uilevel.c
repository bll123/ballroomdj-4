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
#include "level.h"
#include "mdebug.h"
#include "ui.h"
#include "uilevel.h"
#include "uisbtext.h"

typedef struct uilevel {
  level_t       *levels;
  uisbtext_t    *sb;
  bool          allflag;
} uilevel_t;

static const char *uilevelLevelGet (void *udata, int idx);

uilevel_t *
uilevelSpinboxCreate (uiwcont_t *boxp, bool allflag)
{
  uilevel_t  *uilevel;
  int         maxw;


  uilevel = mdmalloc (sizeof (uilevel_t));
  uilevel->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uilevel->allflag = allflag;

  uilevel->sb = uisbtextCreate (boxp);

  maxw = levelGetMaxWidth (uilevel->levels);
  if (allflag) {
    int         len;
    const char  *txt;

    /* CONTEXT: level: a filter: all dance levels will be listed */
    txt = _("All Levels");
    len = istrlen (txt);
    if (len > maxw) {
      maxw = len;
    }
    uisbtextPrependList (uilevel->sb, txt);
  }
  uisbtextSetDisplayCallback (uilevel->sb, uilevelLevelGet, uilevel);
  uisbtextSetCount (uilevel->sb, levelGetCount (uilevel->levels));
  uisbtextSetWidth (uilevel->sb, maxw);

  return uilevel;
}


void
uilevelFree (uilevel_t *uilevel)
{
  if (uilevel == NULL) {
    return;
  }

  uisbtextFree (uilevel->sb);
  mdfree (uilevel);
}

int
uilevelGetValue (uilevel_t *uilevel)
{
  int   idx;

  if (uilevel == NULL) {
    return 0;
  }

  idx = uisbtextGetValue (uilevel->sb);
  return idx;
}

void
uilevelSetValue (uilevel_t *uilevel, int value)
{
  if (uilevel == NULL) {
    return;
  }

  uisbtextSetValue (uilevel->sb, value);
}

void
uilevelSetState (uilevel_t *uilevel, int state)
{
  if (uilevel == NULL || uilevel->sb == NULL) {
    return;
  }
  uisbtextSetState (uilevel->sb, state);
}

void
uilevelSizeGroupAdd (uilevel_t *uilevel, uiwcont_t *sg)
{
  uisbtextSizeGroupAdd (uilevel->sb, sg);
}

void
uilevelSetChangedCallback (uilevel_t *uilevel, callback_t *cb)
{
  uisbtextSetChangeCallback (uilevel->sb, cb);
}

/* internal routines */

static const char *
uilevelLevelGet (void *udata, int idx)
{
  uilevel_t  *uilevel = udata;

  if (idx == -1) {
    if (uilevel->allflag) {
      /* CONTEXT: level: a filter: all dance levels are displayed in the song selection */
      return _("All Levels");
    }
    idx = 0;
  }
  return levelGetLevel (uilevel->levels, idx);
}
