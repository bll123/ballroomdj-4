/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 *
 * generic spinbox
 *    this is the container and buttons for a spinbox.
 *    The display/entry field is passed in by the caller.
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bdj4intl.h"
#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "sysvars.h"
#include "ui.h"
#include "uiclass.h"
#include "uisb.h"
#include "uisbtext.h"
#include "uiwcont.h"

enum {
  SB_W_HBOX,
  SB_W_B_INCR,
  SB_W_B_DECR,
  SB_W_VAL_EVENTH,
  SB_W_MAX,
};

enum {
  SB_CB_INCR,
  SB_CB_DECR,
  SB_CB_VALUE_KEY,
  SB_CB_MAX,
};

typedef struct uisb {
  uiwcont_t       *wcont [SB_W_MAX];
  uiwcont_t       *display;
  callback_t      *callbacks [SB_CB_MAX];
  callback_t      *cb;
  bool            istext;
} uisb_t;

static bool uisbCBIncrement (void *udata);
static bool uisbCBDecrement (void *udata);
static bool uisbKeyEvent (void *udata);
static void uisbSetFocus (uisb_t *sb);

uisb_t *
uisbCreate (uiwcont_t *box, uiwcont_t *disp, bool istext, int margin)
{
  uisb_t      *sb;
  uiwcont_t   *uiwidgetp;

  sb = mdmalloc (sizeof (uisb_t));
  for (int i = 0; i < SB_W_MAX; ++i) {
    sb->wcont [i] = NULL;
  }
  sb->display = NULL;
  for (int i = 0; i < SB_CB_MAX; ++i) {
    sb->callbacks [i] = NULL;
  }
  sb->cb = NULL;
  sb->wcont [SB_W_VAL_EVENTH] = uiEventAlloc ();
  sb->istext = istext;

  uiwidgetp = uiCreateHorizBox ();
  uiWidgetSetClass (uiwidgetp, SB_CLASS);
  if (sb->istext == SB_IS_TEXT) {
    uiWidgetEnableFocus (uiwidgetp);
  }
  uiWidgetSetMarginStart (uiwidgetp, margin);
  sb->wcont [SB_W_HBOX] = uiwidgetp;

  sb->callbacks [SB_CB_INCR] = callbackInit (uisbCBIncrement, sb, "sb-incr");
  sb->callbacks [SB_CB_DECR] = callbackInit (uisbCBDecrement, sb, "sb-decr");
  sb->callbacks [SB_CB_VALUE_KEY] = callbackInit (uisbKeyEvent, sb, "sb-val-key");

  uiwidgetp = uiCreateButton (sb->callbacks [SB_CB_INCR],
      NULL, "sb_incr", NULL);
  uiWidgetSetClass (uiwidgetp, SB_CLASS);
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiWidgetAlignVertCenter (uiwidgetp);
  uiWidgetSetAllMargins (uiwidgetp, 0);
  uiWidgetDisableFocus (uiwidgetp);
  uiBoxPackEnd (sb->wcont [SB_W_HBOX], uiwidgetp, WCONT_KEEP);
  sb->wcont [SB_W_B_INCR] = uiwidgetp;

  uiwidgetp = uiCreateButton (sb->callbacks [SB_CB_DECR],
      NULL, "sb_decr", NULL);
  uiWidgetSetClass (uiwidgetp, SB_CLASS);
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiWidgetAlignVertCenter (uiwidgetp);
  uiWidgetSetAllMargins (uiwidgetp, 0);
  uiWidgetDisableFocus (uiwidgetp);
  uiBoxPackEnd (sb->wcont [SB_W_HBOX], uiwidgetp, WCONT_KEEP);
  sb->wcont [SB_W_B_DECR] = uiwidgetp;

  sb->display = disp;
  uiWidgetSetClass (sb->display, SB_CLASS);
  uiBoxPackStartExpandChildren (sb->wcont [SB_W_HBOX], sb->display, WCONT_KEEP);

  uiBoxPostProcess (sb->wcont [SB_W_HBOX]);
  uiBoxPackStart (box, sb->wcont [SB_W_HBOX], WCONT_KEEP);

  if (sb->istext == SB_IS_TEXT) {
    uiEventSetKeyCallback (sb->wcont [SB_W_VAL_EVENTH], sb->wcont [SB_W_HBOX],
        sb->callbacks [SB_CB_VALUE_KEY]);
  } else {
    uiEventSetKeyCallback (sb->wcont [SB_W_VAL_EVENTH], sb->display,
        sb->callbacks [SB_CB_VALUE_KEY]);
  }

  return sb;
}

void
uisbFree (uisb_t *sb)
{
  if (sb == NULL) {
    return;
  }

  for (int i = 0; i < SB_W_MAX; ++i) {
    uiwcontFree (sb->wcont [i]);
  }
  for (int i = 0; i < SB_CB_MAX; ++i) {
    callbackFree (sb->callbacks [i]);
  }
  mdfree (sb);
}

void
uisbSetCallback (uisb_t *sb, callback_t *uicb)
{
  if (sb == NULL) {
    return;
  }

  sb->cb = uicb;
}

void
uisbSetFocusCallback (uisb_t *sb, callback_t *cb)
{
  uiButtonSetFocusCallback (sb->wcont [SB_W_B_INCR], cb);
  uiButtonSetFocusCallback (sb->wcont [SB_W_B_DECR], cb);
}

void
uisbSetState (uisb_t *sb, int state)
{
  if (sb == NULL) {
    return;
  }

  uiWidgetSetState (sb->display, state);
  uiWidgetSetState (sb->wcont [SB_W_B_INCR], state);
  uiWidgetSetState (sb->wcont [SB_W_B_DECR], state);
}

void
uisbSizeGroupAdd (uisb_t *sb, uiwcont_t *sg)
{
  if (sb == NULL || sg == NULL) {
    return;
  }

  uiSizeGroupAdd (sg, sb->wcont [SB_W_HBOX]);
}

void
uisbSetRepeat (uisb_t *sb, int repeatms)
{
  if (sb == NULL) {
    return;
  }

  uiButtonSetRepeat (sb->wcont [SB_W_B_INCR], repeatms);
  uiButtonSetRepeat (sb->wcont [SB_W_B_DECR], repeatms);
}

void
uisbCheck (uisb_t *sb)
{
  if (sb == NULL) {
    return;
  }

  uiButtonCheckRepeat (sb->wcont [SB_W_B_INCR]);
  uiButtonCheckRepeat (sb->wcont [SB_W_B_DECR]);
  callbackHandlerI (sb->cb, SB_VALIDATE);
}

void
uisbSetFocusHighlight (uisb_t *sb)
{
  if (sb == NULL) {
    return;
  }

  uiWidgetSetClass (sb->wcont [SB_W_HBOX], SB_FOCUS_CLASS);
}

void
uisbClearFocusHighlight (uisb_t *sb)
{
  if (sb == NULL) {
    return;
  }

  uiWidgetClearClass (sb->wcont [SB_W_HBOX], SB_FOCUS_CLASS);
}

/* internal routines */

static bool
uisbCBIncrement (void *udata)
{
  uisb_t    *sb = udata;
  int       rc;

  if (sb->cb == NULL) {
    return UICB_STOP;
  }

  uisbSetFocus (sb);
  rc = callbackHandlerI (sb->cb, SB_INCREMENT);
  return rc;
}

static bool
uisbCBDecrement (void *udata)
{
  uisb_t    *sb = udata;
  int       rc;

  if (sb->cb == NULL) {
    return UICB_STOP;
  }

  uisbSetFocus (sb);
  rc = callbackHandlerI (sb->cb, SB_DECREMENT);
  return rc;
}

static bool
uisbKeyEvent (void *udata)
{
  uisb_t    *sb = udata;

  logProcBegin ();
  if (sb == NULL) {
    logProcEnd ("bad-sb");
    return UICB_CONT;
  }

  /* need to grab the press event as otherwise gtk grabs the key */
  if (uiEventIsMovementKey (sb->wcont [SB_W_VAL_EVENTH]) &&
      uiEventIsKeyPressEvent (sb->wcont [SB_W_VAL_EVENTH])) {
    int     inctype;
    int     rc;

    inctype = SB_INCREMENT;
    if (uiEventIsPageUpDownKey (sb->wcont [SB_W_VAL_EVENTH])) {
      inctype = SB_PAGE_INCR;
    }
    if (uiEventIsDownKey (sb->wcont [SB_W_VAL_EVENTH])) {
      inctype = - inctype;
    }
    uisbSetFocus (sb);
    rc = callbackHandlerI (sb->cb, inctype);

    logProcEnd ("up-down");
    /* do not want gtk to process this key */
    return UICB_STOP;
  }

  if (uiEventIsKeyPressEvent (sb->wcont [SB_W_VAL_EVENTH])) {
    if (uiEventIsNavKey (sb->wcont [SB_W_VAL_EVENTH]) ||
        uiEventIsEnterKey (sb->wcont [SB_W_VAL_EVENTH])) {
      int   rc;

      rc = callbackHandlerI (sb->cb, SB_VAL_FORCE);
      if (uiEventIsNavKey (sb->wcont [SB_W_VAL_EVENTH])) {
        return UICB_CONT;
      }
      return rc;
    }
  }

  logProcEnd ("");
  return UICB_CONT;
}

static void
uisbSetFocus (uisb_t *sb)
{
  if (sb->istext == SB_IS_TEXT) {
    uiWidgetGrabFocus (sb->wcont [SB_W_HBOX]);
  } else {
    uiWidgetGrabFocus (sb->display);
  }
}
