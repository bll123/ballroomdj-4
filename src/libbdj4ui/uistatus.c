/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
#include "status.h"
#include "ui.h"
#include "uistatus.h"

typedef struct uistatus {
  status_t      *status;
  uiwcont_t     *spinbox;
  bool          allflag;
} uistatus_t;

static const char *uistatusStatusGet (void *udata, int idx);

uistatus_t *
uistatusSpinboxCreate (uiwcont_t *boxp, bool allflag)
{
  uistatus_t  *uistatus;
  int         maxw;
  int         start;
  int         len;


  uistatus = mdmalloc (sizeof (uistatus_t));
  uistatus->status = bdjvarsdfGet (BDJVDF_STATUS);
  uistatus->allflag = allflag;
  uistatus->spinbox = uiSpinboxInit ();

  uiSpinboxTextCreate (uistatus->spinbox, uistatus);

  start = 0;
  maxw = statusGetMaxWidth (uistatus->status);
  if (allflag) {
    /* CONTEXT: status: a filter: all dance status will be listed */
    len = istrlen (_("Any Status"));
    if (len > maxw) {
      maxw = len;
    }
    start = -1;
  }
  uiSpinboxTextSet (uistatus->spinbox, start,
      statusGetCount (uistatus->status),
      maxw, NULL, NULL, uistatusStatusGet);

  uiBoxPackStart (boxp, uistatus->spinbox);

  return uistatus;
}


void
uistatusFree (uistatus_t *uistatus)
{
  if (uistatus == NULL) {
    return;
  }

  uiwcontFree (uistatus->spinbox);
  mdfree (uistatus);
}

int
uistatusGetValue (uistatus_t *uistatus)
{
  int   idx;

  if (uistatus == NULL) {
    return 0;
  }

  idx = uiSpinboxTextGetValue (uistatus->spinbox);
  return idx;
}

void
uistatusSetValue (uistatus_t *uistatus, int value)
{
  if (uistatus == NULL) {
    return;
  }

  uiSpinboxTextSetValue (uistatus->spinbox, value);
}

void
uistatusSetState (uistatus_t *uistatus, int state)
{
  if (uistatus == NULL || uistatus->spinbox == NULL) {
    return;
  }
  uiSpinboxSetState (uistatus->spinbox, state);
}

void
uistatusSizeGroupAdd (uistatus_t *uistatus, uiwcont_t *sg)
{
  uiSizeGroupAdd (sg, uistatus->spinbox);
}

void
uistatusSetChangedCallback (uistatus_t *uistatus, callback_t *cb)
{
  uiSpinboxTextSetValueChangedCallback (uistatus->spinbox, cb);
}

/* internal routines */

static const char *
uistatusStatusGet (void *udata, int idx)
{
  uistatus_t  *uistatus = udata;

  if (idx == -1) {
    if (uistatus->allflag) {
      /* CONTEXT: status: a filter: all dance status are displayed in the song selection */
      return _("Any Status");
    }
    idx = 0;
  }
  return statusGetStatus (uistatus->status, idx);
}

