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
#include "istring.h"
#include "log.h"
#include "mdebug.h"
#include "tmutil.h"
#include "ui.h"
#include "uisb.h"
#include "uisbnum.h"
#include "uiwcont.h"
#include "validate.h"

static double SB_INVALID = -65534;

enum {
  SBNUM_CB_MAIN,
  SBNUM_CB_ENTRY_IN,
  SBNUM_CB_ENTRY_OUT,
  SBNUM_CB_MAX,
};

typedef struct uisbnum {
  uisb_t          *sb;
  uiwcont_t       *entry;
  callback_t      *callbacks [SBNUM_CB_MAX];
  callback_t      *chgcb;
  const char      *label;
  const char      *dfltstr;
  size_t          dfltlen;
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
  bool            isvalid;
  char            fmt [20];
  char            valmsg [400];
} uisbnum_t;

static bool uisbnumCBHandler (void *udata, int32_t dir);
static void uisbnumSetDisplay (uisbnum_t *sbnum);
static void uisbnumSetFormat (uisbnum_t *sbnum);
static int uisbnumEntryValidate (uiwcont_t *entry, const char *label, void *udata);
static void uisbnumValueToStr (uisbnum_t *sbnum, char *tbuff, size_t sz);
static void uisbnumProcessChangeCallback (uisbnum_t *sbnum);
static void uisbnumSetValid (uisbnum_t *sbnum, bool isvalid);
static bool uisbnumFocusHandler (void *udata);
static bool uisbnumFocusOutHandler (void *udata);

uisbnum_t *
uisbnumCreate (uiwcont_t *box, const char *label, int margin)
{
  uisbnum_t  *sbnum;

  sbnum = mdmalloc (sizeof (uisbnum_t));
  sbnum->entry = uiEntryInit (6, 10);
  uiWidgetAlignHorizFill (sbnum->entry);
  uiEntryAlignEnd (sbnum->entry);
  uiWidgetSetAllMargins (sbnum->entry, 0);
  sbnum->sb = uisbCreate (box, sbnum->entry, SB_IS_NUM, margin);
  uisbSetRepeat (sbnum->sb, 50);
  for (int i = 0; i < SBNUM_CB_MAX; ++i) {
    sbnum->callbacks [i] = NULL;
  }
  sbnum->chgcb = NULL;
  sbnum->label = label;
  /* CONTEXT: user interface: default setting display */
  sbnum->dfltstr = _("Default");
  sbnum->dfltlen = istrlen (sbnum->dfltstr);
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
  sbnum->isvalid = true;
  uisbnumSetFormat (sbnum);
  sbnum->valmsg [0] = '\0';

  sbnum->callbacks [SBNUM_CB_MAIN] = callbackInitI (uisbnumCBHandler, sbnum);
  uisbSetCallback (sbnum->sb, sbnum->callbacks [SBNUM_CB_MAIN]);
  sbnum->callbacks [SBNUM_CB_ENTRY_IN] =
      callbackInit (uisbnumFocusHandler, sbnum, NULL);
  uiEntrySetFocusCallback (sbnum->entry, sbnum->callbacks [SBNUM_CB_ENTRY_IN]);
  sbnum->callbacks [SBNUM_CB_ENTRY_OUT] =
      callbackInit (uisbnumFocusOutHandler, sbnum, NULL);
  uiEntrySetFocusOutCallback (sbnum->entry, sbnum->callbacks [SBNUM_CB_ENTRY_OUT]);

  uiEntrySetValidate (sbnum->entry, label,
      uisbnumEntryValidate, sbnum, UIENTRY_DELAY_NO_ICON);

  return sbnum;
}

void
uisbnumFree (uisbnum_t *sbnum)
{
  if (sbnum == NULL) {
    return;
  }

  uisbFree (sbnum->sb);
  for (int i = 0; i < SBNUM_CB_MAX; ++i) {
    callbackFree (sbnum->callbacks [i]);
  }
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
    /* h:mm:ss */
    uiEntrySetWidth (sbnum->entry, 7);
  }
  if (timetype == SBNUM_TIME_PRECISE) {
    sbnum->valtype = VAL_HMS_PRECISE | VAL_NOT_EMPTY;
    sbnum->incr = 100.0;
    sbnum->pageincr = 30000.0;
    /* mm:ss.v */
    uiEntrySetWidth (sbnum->entry, 7);
  }
}

/* be sure to call this after setting the limits */
void
uisbnumSetType (uisbnum_t *sbnum, int type)
{
  if (sbnum == NULL) {
    return;
  }

  if (type != SBNUM_NUM_DEFAULT) {
    /* other type changes are not supported */
    return;
  }

  sbnum->type = type;
  sbnum->min = - sbnum->incr;
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
uisbnumResetChanged (uisbnum_t *sbnum)
{
  if (sbnum == NULL) {
    return;
  }

  sbnum->changed = false;
}

void
uisbnumSetChangeCallback (uisbnum_t *sbnum, callback_t *chgcb)
{
  if (sbnum == NULL) {
    return;
  }

  sbnum->chgcb = chgcb;
}

void
uisbnumValidate (uisbnum_t *sbnum)
{
  if (sbnum == NULL) {
    return;
  }

  uiEntryValidate (sbnum->entry, false);
}

void
uisbnumSetFocusCallback (uisbnum_t *sbnum, callback_t *cb)
{
  if (sbnum == NULL) {
    return;
  }

  uiEntrySetFocusCallback (sbnum->entry, cb);
  uisbSetFocusCallback (sbnum->sb, cb);
}

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


  if (sbnum->old_value != SB_INVALID &&
      sbnum->old_value != value) {
    sbnum->changed = true;
  }
  sbnum->old_value = value;
  sbnum->value = value;
  uisbnumSetDisplay (sbnum);
}

double
uisbnumGetValue (uisbnum_t *sbnum)
{
  if (sbnum == NULL) {
    return LIST_VALUE_INVALID;
  }

  return sbnum->value;
}

bool
uisbnumIsValid (uisbnum_t *sbnum)
{
  if (sbnum == NULL) {
    return false;
  }

  return sbnum->isvalid;
}

const char *
uisbnumGetValidationError (uisbnum_t *sbnum)
{
  if (sbnum == NULL) {
    return "";
  }

  return sbnum->valmsg;
}

/* internal routines */

static bool
uisbnumCBHandler (void *udata, int32_t dir)
{
  uisbnum_t  *sbnum = udata;

  if (sbnum == NULL) {
    return UICB_CONT;
  }

  if (dir == SB_VALIDATE || dir == SB_VAL_FORCE) {
    int     rc;
    bool    force = false;

    if (dir == SB_VAL_FORCE) {
      force = true;
    }

    rc = uiEntryValidate (sbnum->entry, force);
    if (rc == UIENTRY_OK) {
      uisbnumSetValid (sbnum, true);
      return UICB_CONT;
    } else {
      uisbnumSetValid (sbnum, false);
      if (force) {
        char    tbuff [40];

        uisbnumValueToStr (sbnum, tbuff, sizeof (tbuff));
        uiEntrySetValue (sbnum->entry, tbuff);
        uisbnumSetValid (sbnum, true);
      }
    }
    return UICB_STOP;
  }

  if (dir == SB_INCREMENT) {
    sbnum->value += sbnum->incr;
  }
  if (dir == SB_DECREMENT) {
    sbnum->value -= sbnum->incr;
  }
  if (dir == SB_PAGE_INCR) {
    sbnum->value += sbnum->pageincr;
  }
  if (dir == SB_PAGE_DECR) {
    sbnum->value -= sbnum->pageincr;
  }

  uisbnumSetDisplay (sbnum);

  return UICB_CONT;
}

static bool
uisbnumFocusHandler (void *udata)
{
  uisbnum_t  *sbnum = udata;

  if (sbnum == NULL) {
    return UICB_CONT;
  }

  uisbSetFocusHighlight (sbnum->sb);
  return UICB_CONT;
}

static bool
uisbnumFocusOutHandler (void *udata)
{
  uisbnum_t  *sbnum = udata;

  if (sbnum == NULL) {
    return UICB_CONT;
  }

  uisbClearFocusHighlight (sbnum->sb);
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
  if (sbnum->type == SBNUM_NUM_DEFAULT) {
    if (sbnum->value < 0) {
      sbnum->value = - sbnum->incr;
    }
  }

  if (sbnum->old_value != SB_INVALID &&
      sbnum->old_value != sbnum->value) {
    sbnum->changed = true;
  }

  uisbnumValueToStr (sbnum, tbuff, sizeof (tbuff));
  sbnum->set_value = true;
  uiEntrySetValue (sbnum->entry, tbuff);
  uiEntryValidateClear (sbnum->entry);
  uisbnumProcessChangeCallback (sbnum);
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
  const char  *str;

  if (sbnum == NULL) {
    return UIENTRY_ERROR;
  }

  if (sbnum->set_value) {
    sbnum->isvalid = true;
    return UIENTRY_OK;
  }

  str = uiEntryGetValue (sbnum->entry);
  if (! validate (sbnum->valmsg, sizeof (sbnum->valmsg),
        label, str, sbnum->valtype)) {
    uisbnumSetValid (sbnum, false);
    rc = UIENTRY_ERROR;
  }
  if (rc == UIENTRY_OK) {
    if (sbnum->type == SBNUM_NUMERIC) {
      sbnum->value = atof (str);
    }
    if (sbnum->type == SBNUM_NUM_DEFAULT) {
      if (sbnum->value < 0) {
        sbnum->value = - sbnum->incr;
      } else {
        sbnum->value = atof (str);
      }
    }
    if (sbnum->type == SBNUM_TIME_BASIC ||
        sbnum->type == SBNUM_TIME_PRECISE) {
      sbnum->value = tmutilStrToMS (str);
    }
    sbnum->changed = true;
    uisbnumSetValid (sbnum, true);
  }

  uisbnumProcessChangeCallback (sbnum);

  return rc;
}

static void
uisbnumValueToStr (uisbnum_t *sbnum, char *tbuff, size_t sz)
{
  if (sbnum->type == SBNUM_NUM_DEFAULT) {
    if (sbnum->value < 0) {
      stpecpy (tbuff, tbuff + sz, sbnum->dfltstr);
      uiEntrySetWidth (sbnum->entry, sbnum->dfltlen + 1);
    } else {
      snprintf (tbuff, sz, sbnum->fmt, sbnum->value);
    }
  }
  if (sbnum->type == SBNUM_NUMERIC) {
    snprintf (tbuff, sz, sbnum->fmt, sbnum->value);
  }
  if (sbnum->type == SBNUM_TIME_BASIC) {
    tmutilToMS ((time_t) sbnum->value, tbuff, sz);
  }
  if (sbnum->type == SBNUM_TIME_PRECISE) {
    tmutilToMSD ((time_t) sbnum->value, tbuff, sz, sbnum->digits);
  }
}

static void
uisbnumProcessChangeCallback (uisbnum_t *sbnum)
{
  if (sbnum->chgcb != NULL) {
    if (sbnum->changed) {
      callbackHandler (sbnum->chgcb);
      sbnum->changed = false;
    }
  }
}

static void
uisbnumSetValid (uisbnum_t *sbnum, bool isvalid)
{
  if (sbnum->isvalid != isvalid) {
    sbnum->changed = true;
  }
  sbnum->isvalid = isvalid;
  if (sbnum->changed) {
    uisbnumProcessChangeCallback (sbnum);
  }
}
