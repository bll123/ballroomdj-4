/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 *
 * spinbox-text
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "ui.h"
#include "uisb.h"
#include "uisbnum.h"
#include "uiwcont.h"

typedef struct uisbnum {
  uisb_t          *sb;
  uiwcont_t       *entry;
  callback_t      *sbnumcb;
  callback_t      *chgcb;
  double          min;
  double          max;
  int             digits;
  double          value;
  double          incr;
  double          pageincr;
} uisbnum_t;

static bool uisbnumCBHandler (void *udata, int32_t dir);
static void uisbnumSetDisplay (uisbnum_t *sbnum);

uisbnum_t *
uisbnumCreate (uiwcont_t *box, int maxWidth)
{
  uisbnum_t  *sbnum;

  sbnum = mdmalloc (sizeof (uisbnum_t));
  sbnum->entry = uiEntryInit (maxWidth, maxWidth);
  sbnum->sb = uisbCreate (box, sbnum->entry);
  sbnum->value = 0;
  sbnum->sbnumcb = NULL;
  sbnum->chgcb = NULL;

  sbnum->sbnumcb = callbackInitI (uisbnumCBHandler, sbnum);
  uisbSetCallback (sbnum->sb, sbnum->sbnumcb);

  return sbnum;
}

void
uisbnumFree (uisbnum_t *sbnum)
{
  if (sbnum == NULL) {
    return;
  }

  uisbFree (sbnum->sb);
  callbackFree (sbnum->sbnumcb);
  uiwcontFree (sbnum->entry);
  mdfree (sbnum);
}

bool
uisbnumIsChanged (uisbnum_t *sbnum)
{
  bool      chg;

  if (sbnum == NULL) {
    return false;
  }

  chg = uiEntryChanged (sbnum->entry);
  uiEntryClearChanged (sbnum->entry);
  return chg;
}

void
uisbnumSetChangeCallback (uisbnum_t *sbnum, callback_t *chgcb)
{
  if (sbnum == NULL) {
    return;
  }

  sbnum->chgcb = chgcb;
}

#if 0
void
uisbnumAddClass (uisbnum_t *sbnum, const char *name)
{
  if (sbnum == NULL || name == NULL) {
    return;
  }

  uiWidgetAddClass (sbnum->entry, name);
}

void
uisbnumRemoveClass (uisbnum_t *sbnum, const char *name)
{
  if (sbnum == NULL || name == NULL) {
    return;
  }

  uiWidgetRemoveClass (sbnum->entry, name);
}
#endif

void
uisbnumSetState (uisbnum_t *sbnum, int state)
{
  if (sbnum == NULL) {
    return;
  }

  uisbSetState (sbnum->sb, state);
}

void
uisbnumSizeGroupAdd (uisbnum_t *sbnum, uiwcont_t *sg)
{
  if (sbnum == NULL || sg == NULL) {
    return;
  }

  uisbSizeGroupAdd (sbnum->sb, sg);
}

void
uisbnumSetValue (uisbnum_t *sbnum, double value)
{
  if (sbnum == NULL) {
    return;
  }

  sbnum->value = value;
  uisbnumSetDisplay (sbnum);
  uiEntryClearChanged (sbnum->entry);
}

double
uisbnumGetValue (uisbnum_t *sbnum)
{
  if (sbnum == NULL) {
    return LIST_VALUE_INVALID;
  }

  return sbnum->value;
}

/* internal routines */

static bool
uisbnumCBHandler (void *udata, int32_t dir)
{
  uisbnum_t  *sbnum = udata;

  if (sbnum == NULL) {
    return UICB_STOP;
  }

  if (dir == SB_INCREMENT) {
    sbnum->value += sbnum->incr;
  }
  if (dir == SB_DECREMENT) {
    sbnum->value -= sbnum->incr;
  }

  uisbnumSetDisplay (sbnum);

  return UICB_CONT;
}

static void
uisbnumSetDisplay (uisbnum_t *sbnum)
{
  if (sbnum->chgcb != NULL) {
    if (uiEntryChanged (sbnum->entry)) {
      callbackHandler (sbnum->chgcb);
    }
  }

// ### set display...

}
