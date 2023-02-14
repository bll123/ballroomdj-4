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
#include <assert.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "istring.h"
#include "level.h"
#include "mdebug.h"
#include "ui.h"
#include "uilevel.h"

typedef struct uilevel {
  level_t       *levels;
  uispinbox_t   *spinbox;
  bool          allflag;
} uilevel_t;

static const char *uilevelLevelGet (void *udata, int idx);

uilevel_t *
uilevelSpinboxCreate (UIWidget *boxp, bool allflag)
{
  uilevel_t  *uilevel;
  int         maxw;
  int         start;
  int         len;


  uilevel = mdmalloc (sizeof (uilevel_t));
  uilevel->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uilevel->allflag = allflag;
  uilevel->spinbox = uiSpinboxInit ();

  uiSpinboxTextCreate (uilevel->spinbox, uilevel);

  start = 0;
  maxw = levelGetMaxWidth (uilevel->levels);
  if (allflag) {
    /* CONTEXT: level: a filter: all dance levels will be listed */
    len = istrlen (_("All Levels"));
    if (len > maxw) {
      maxw = len;
    }
    start = -1;
  }
  uiSpinboxTextSet (uilevel->spinbox, start,
      levelGetCount (uilevel->levels),
      maxw, NULL, NULL, uilevelLevelGet);

  uiBoxPackStart (boxp, uiSpinboxGetUIWidget (uilevel->spinbox));

  return uilevel;
}


void
uilevelFree (uilevel_t *uilevel)
{
  if (uilevel != NULL) {
    uiSpinboxFree (uilevel->spinbox);
    mdfree (uilevel);
  }
}

int
uilevelGetValue (uilevel_t *uilevel)
{
  int   idx;

  if (uilevel == NULL) {
    return 0;
  }

  idx = uiSpinboxTextGetValue (uilevel->spinbox);
  return idx;
}

void
uilevelSetValue (uilevel_t *uilevel, int value)
{
  if (uilevel == NULL) {
    return;
  }

  uiSpinboxTextSetValue (uilevel->spinbox, value);
}

void
uilevelDisable (uilevel_t *uilevel)
{
  if (uilevel == NULL || uilevel->spinbox == NULL) {
    return;
  }
  uiSpinboxDisable (uilevel->spinbox);
}

void
uilevelEnable (uilevel_t *uilevel)
{
  if (uilevel == NULL || uilevel->spinbox == NULL) {
    return;
  }
  uiSpinboxEnable (uilevel->spinbox);
}

void
uilevelSizeGroupAdd (uilevel_t *uilevel, UIWidget *sg)
{
  uiSizeGroupAdd (sg, uiSpinboxGetUIWidget (uilevel->spinbox));
}

void
uilevelSetChangedCallback (uilevel_t *uilevel, callback_t *cb)
{
  uiSpinboxTextSetValueChangedCallback (uilevel->spinbox, cb);
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
