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

#include "bdjstring.h"
#include "bdj4intl.h"
#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "tmutil.h"
#include "ui.h"
#include "uisb.h"
#include "uisbnum.h"
#include "uiwcont.h"
#include "validate.h"

static double SB_INVALID = -65534;

typedef struct uisbnum {
  uisb_t          *sb;
  uiwcont_t       *entry;
  callback_t      *sbnumcb;
  callback_t      *chgcb;
  double          min;
  double          max;
  double          old_value;
  double          value;
  double          incr;
  double          pageincr;
  int             digits;
  int             valtype;
  int             type;
  bool            changed;
  bool            set_value;
  char            fmt [20];
} uisbnum_t;

static bool uisbnumCBHandler (void *udata, int32_t dir);
static void uisbnumSetDisplay (uisbnum_t *sbnum);
static void uisbnumSetFormat (uisbnum_t *sbnum);
static int uisbnumEntryValidate (uiwcont_t *entry, const char *label, void *udata);

uisbnum_t *
uisbnumCreate (uiwcont_t *box, int maxWidth)
{
  uisbnum_t  *sbnum;

  sbnum = mdmalloc (sizeof (uisbnum_t));
  sbnum->entry = uiEntryInit (maxWidth, maxWidth);
  uiEntryAlignEnd (sbnum->entry);
  uiWidgetSetAllMargins (sbnum->entry, 0);
  sbnum->sb = uisbCreate (box, sbnum->entry);
  uisbSetRepeat (sbnum->sb, 50);
  sbnum->sbnumcb = NULL;
  sbnum->chgcb = NULL;
  sbnum->min = 1.0;
  sbnum->max = 100.0;
  sbnum->old_value = SB_INVALID;
  sbnum->value = 0;
  sbnum->incr = 1.0;
  sbnum->pageincr = 5.0;
  sbnum->digits = 1;
  sbnum->valtype = VAL_NUMERIC | VAL_NOT_EMPTY;
  sbnum->type = SBNUM_NUMERIC;
  sbnum->changed = false;
  sbnum->set_value = false;
  uisbnumSetFormat (sbnum);

  sbnum->sbnumcb = callbackInitI (uisbnumCBHandler, sbnum);
  uisbSetCallback (sbnum->sb, sbnum->sbnumcb);

  uiEntrySetValidate (sbnum->entry, "",
      uisbnumEntryValidate, sbnum, UIENTRY_IMMEDIATE);

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

void
uisbnumSetIncrements (uisbnum_t *sbnum, double incr, double pageincr)
{
  if (sbnum == NULL) {
    return;
  }

  sbnum->incr = incr;
  sbnum->pageincr = pageincr;
}

void
uisbnumSetLimits (uisbnum_t *sbnum, double min, double max, int digits)
{
  if (sbnum == NULL) {
    return;
  }

  sbnum->min = min;
  sbnum->max = max;
  sbnum->type = SBNUM_NUMERIC;
  sbnum->digits = digits;
  sbnum->valtype = VAL_NUMERIC | VAL_NOT_EMPTY;
  if (digits > 0) {
    sbnum->valtype = VAL_FLOAT | VAL_NOT_EMPTY;
  }

  uisbnumSetFormat (sbnum);
}

void
uisbnumSetTime (uisbnum_t *sbnum, double min, double max, int timetype)
{
  if (sbnum == NULL) {
    return;
  }

  sbnum->min = min;
  sbnum->max = max;
  sbnum->type = timetype;
  if (timetype == SBNUM_TIME_BASIC) {
    sbnum->valtype = VAL_HMS | VAL_NOT_EMPTY;
    sbnum->incr = 5000.0;
    sbnum->pageincr = 60000.0;
  }
  if (timetype == SBNUM_TIME_PRECISE) {
    sbnum->valtype = VAL_HMS_PRECISE | VAL_NOT_EMPTY;
    sbnum->incr = 100.0;
    sbnum->pageincr = 30000.0;
  }
}

void
uisbnumCheck (uisbnum_t *sbnum)
{
  if (sbnum == NULL) {
    return;
  }

  uisbCheck (sbnum->sb);
}

bool
uisbnumIsChanged (uisbnum_t *sbnum)
{
  bool      chg;

  if (sbnum == NULL) {
    return false;
  }

  chg = sbnum->changed;
  sbnum->changed = false;
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

  sbnum->old_value = SB_INVALID;
  sbnum->value = value;
  uisbnumSetDisplay (sbnum);
  sbnum->changed = false;
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

  uiWidgetGrabFocus (sbnum->entry);
  uisbnumSetDisplay (sbnum);

  return UICB_CONT;
}

static void
uisbnumSetDisplay (uisbnum_t *sbnum)
{
  char    tbuff [40];

  if (sbnum->value < sbnum->min) {
    sbnum->value = sbnum->min;
  }
  if (sbnum->value > sbnum->max) {
    sbnum->value = sbnum->max;
  }

  if (sbnum->old_value != SB_INVALID &&
      sbnum->old_value != sbnum->value) {
    sbnum->changed = true;
  }

  if (sbnum->chgcb != NULL) {
    if (sbnum->changed) {
      callbackHandler (sbnum->chgcb);
      sbnum->changed = false;
    }
  }

  if (sbnum->type == SBNUM_NUMERIC) {
    if (sbnum->value < 0) {
      /* CONTEXT: user interface: default setting display */
      stpecpy (tbuff, tbuff + sizeof (tbuff), _("Default"));
    } else {
      snprintf (tbuff, sizeof (tbuff), sbnum->fmt, sbnum->value);
    }
  }
  if (sbnum->type == SBNUM_TIME_BASIC) {
    tmutilToMS ((time_t) sbnum->value, tbuff, sizeof (tbuff));
  }
  if (sbnum->type == SBNUM_TIME_PRECISE) {
    tmutilToMSD ((time_t) sbnum->value, tbuff, sizeof (tbuff), sbnum->digits);
  }

  sbnum->set_value = true;
  uiEntrySetValue (sbnum->entry, tbuff);
  sbnum->set_value = false;
}

static void
uisbnumSetFormat (uisbnum_t *sbnum)
{
  snprintf (sbnum->fmt, sizeof (sbnum->fmt), "%%.%df", sbnum->digits);
}

static int
uisbnumEntryValidate (uiwcont_t *entry, const char *label, void *udata)
{
  uisbnum_t   *sbnum = udata;
  int         rc = UIENTRY_OK;
  char        msg [200];
  const char  *str;

  if (sbnum->set_value) {
    return UIENTRY_OK;
  }

  str = uiEntryGetValue (sbnum->entry);
  if (! validate (msg, sizeof (msg), label, str, sbnum->valtype)) {
    rc = UIENTRY_ERROR;
  }
  if (rc == UIENTRY_OK) {
    if (sbnum->type == SBNUM_NUMERIC) {
      sbnum->value = atof (str);
    }
    if (sbnum->type == SBNUM_TIME_BASIC ||
        sbnum->type == SBNUM_TIME_PRECISE) {
      sbnum->value = tmutilStrToMS (str);
    }
    sbnum->changed = true;
  }

  return rc;
}
