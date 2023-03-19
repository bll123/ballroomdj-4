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
#include "dance.h"
#include "mdebug.h"
#include "ui.h"
#include "callback.h"
#include "uidance.h"

typedef struct uidance {
  dance_t       *dances;
  uidropdown_t  *dropdown;
  UIWidget      *parentwin;
  UIWidget      *buttonp;
  callback_t    *cb;
  callback_t    *selectcb;
  const char    *label;
  long          selectedidx;
  int           count;
  int           flags;
} uidance_t;

static bool uidanceSelectHandler (void *udata, long idx);
static void uidanceCreateDanceList (uidance_t *uidance);

uidance_t *
uidanceDropDownCreate (UIWidget *boxp, uiwidget_t *parentwin, int flags,
    const char *label, int where, int count)
{
  uidance_t  *uidance;


  uidance = mdmalloc (sizeof (uidance_t));
  uidance->dances = bdjvarsdfGet (BDJVDF_DANCES);
  uidance->count = count;
  uidance->flags = flags;
  uidance->dropdown = uiDropDownInit ();
  uidance->selectedidx = 0;
  uidance->parentwin = parentwin;
  uidance->cb = NULL;
  uidance->selectcb = NULL;

  uidance->cb = callbackInitLong (uidanceSelectHandler, uidance);
  uidance->label = label;  /* this is a temporary value */
  if (flags == UIDANCE_NONE) {
    uidance->buttonp = uiDropDownCreate (parentwin, label,
        uidance->cb, uidance->dropdown, uidance);
  } else {
    /* UIDANCE_ALL_DANCES or UIDANCE_EMPTY_DANCE */
    uidance->buttonp = uiComboboxCreate (parentwin, label,
        uidance->cb, uidance->dropdown, uidance);
  }
  uidanceCreateDanceList (uidance);
  if (where == UIDANCE_PACK_END) {
    uiBoxPackEnd (boxp, uidance->buttonp);
  }
  if (where == UIDANCE_PACK_START) {
    uiBoxPackStart (boxp, uidance->buttonp);
  }

  return uidance;
}

UIWidget *
uidanceGetButton (uidance_t *uidance)
{
  if (uidance == NULL) {
    return NULL;
  }
  return uidance->buttonp;
}


void
uidanceFree (uidance_t *uidance)
{
  if (uidance != NULL) {
    callbackFree (uidance->cb);
    uiDropDownFree (uidance->dropdown);
    mdfree (uidance);
  }
}

int
uidanceGetValue (uidance_t *uidance)
{
  if (uidance == NULL) {
    return 0;
  }

  return uidance->selectedidx;
}

void
uidanceSetValue (uidance_t *uidance, int value)
{
  if (uidance == NULL || uidance->dropdown == NULL) {
    return;
  }

  uidance->selectedidx = value;
  uiDropDownSelectionSetNum (uidance->dropdown, value);
}

void
uidanceSetState (uidance_t *uidance, int state)
{
  if (uidance == NULL || uidance->dropdown == NULL) {
    return;
  }
  uiDropDownSetState (uidance->dropdown, state);
}

void
uidanceSizeGroupAdd (uidance_t *uidance, uiwidget_t *sg)
{
  uiSizeGroupAdd (sg, uidance->buttonp);
}

void
uidanceSetCallback (uidance_t *uidance, callback_t *cb)
{
  if (uidance == NULL) {
    return;
  }
  uidance->selectcb = cb;
}


/* internal routines */

static bool
uidanceSelectHandler (void *udata, long idx)
{
  uidance_t   *uidance = udata;

  uidance->selectedidx = idx;
  if (uidance->flags != UIDANCE_NONE) {
    /* the drop down set must be called to set the combobox display */
    uiDropDownSelectionSetNum (uidance->dropdown, idx);
  }

  if (uidance->selectcb != NULL) {
    callbackHandlerLongInt (uidance->selectcb, idx, uidance->count);
  }
  return UICB_CONT;
}

static void
uidanceCreateDanceList (uidance_t *uidance)
{
  slist_t     *danceList;
  const char  *selectLabel;

  danceList = danceGetDanceList (uidance->dances);
  selectLabel = NULL;
  /* if it is a combobox (UIDANCE_ALL_DANCES, UIDANCE_EMPTY_DANCE) */
  if (uidance->flags != UIDANCE_NONE) {
    selectLabel = uidance->label;
  }
  uiDropDownSetNumList (uidance->dropdown, danceList, selectLabel);
}

