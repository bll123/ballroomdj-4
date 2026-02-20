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
#include "mdebug.h"
#include "status.h"
#include "ui.h"
#include "uisbtext.h"
#include "uistatus.h"

typedef struct uistatus {
  status_t      *status;
  uisbtext_t    *sb;
  bool          allflag;
} uistatus_t;

static const char *uistatusStatusGet (void *udata, int idx);

uistatus_t *
uistatusSpinboxCreate (uiwcont_t *boxp, bool allflag)
{
  uistatus_t  *uistatus;
  int         maxw;


  uistatus = mdmalloc (sizeof (uistatus_t));
  uistatus->status = bdjvarsdfGet (BDJVDF_STATUS);
  uistatus->allflag = allflag;
  uistatus->sb = uisbtextCreate (boxp);

  maxw = statusGetMaxWidth (uistatus->status);
  if (allflag) {
    const char  *txt;
    int         len;

    /* CONTEXT: status: a filter: all dance status will be listed */
    txt = _("Any Status");
    len = istrlen (txt);
    if (len > maxw) {
      maxw = len;
    }
    uisbtextPrependList (uistatus->sb, txt);
  }
  uisbtextSetDisplayCallback (uistatus->sb, uistatusStatusGet, uistatus);
  uisbtextSetCount (uistatus->sb, statusGetCount (uistatus->status));

  return uistatus;
}


void
uistatusFree (uistatus_t *uistatus)
{
  if (uistatus == NULL) {
    return;
  }

  uisbtextFree (uistatus->sb);
  mdfree (uistatus);
}

int
uistatusGetValue (uistatus_t *uistatus)
{
  int   idx;

  if (uistatus == NULL) {
    return 0;
  }

  idx = uisbtextGetValue (uistatus->sb);
  return idx;
}

void
uistatusSetValue (uistatus_t *uistatus, int value)
{
  if (uistatus == NULL) {
    return;
  }

  uisbtextSetValue (uistatus->sb, value);
}

void
uistatusSetState (uistatus_t *uistatus, int state)
{
  if (uistatus == NULL || uistatus->sb == NULL) {
    return;
  }
  uisbtextSetState (uistatus->sb, state);
}

void
uistatusSizeGroupAdd (uistatus_t *uistatus, uiwcont_t *sg)
{
  if (uistatus == NULL || uistatus->sb == NULL) {
    return;
  }

  uisbtextSizeGroupAdd (uistatus->sb, sg);
}

void
uistatusSetChangedCallback (uistatus_t *uistatus, callback_t *cb)
{
  uisbtextSetChangeCallback (uistatus->sb, cb);
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

