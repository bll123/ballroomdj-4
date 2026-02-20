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
#include "ui.h"
#include "uiclass.h"
#include "uisb.h"
#include "uisbtext.h"
#include "uiwcont.h"

typedef struct uisb {
  uiwcont_t       *hbox;
  uiwcont_t       *incr;
  uiwcont_t       *decr;
  uiwcont_t       *display;
  callback_t      *sbcbincr;
  callback_t      *sbcbdecr;
  callback_t      *cb;
} uisb_t;

static bool uisbCBIncrement (void *udata);
static bool uisbCBDecrement (void *udata);

uisb_t *
uisbCreate (uiwcont_t *box, uiwcont_t *disp)
{
  uisb_t      *sb;

  sb = mdmalloc (sizeof (uisb_t));
  sb->hbox = NULL;
  sb->incr = NULL;
  sb->decr = NULL;
  sb->display = NULL;
  sb->sbcbincr = NULL;
  sb->sbcbdecr = NULL;
  sb->cb = NULL;

  sb->hbox = uiCreateHorizBox ();
  uiWidgetAddClass (sb->hbox, SB_CLASS);
  sb->sbcbincr = callbackInit (uisbCBIncrement, sb, "sb-incr");
  sb->sbcbdecr = callbackInit (uisbCBDecrement, sb, "sb-decr");

  sb->incr = uiCreateButton (sb->sbcbincr, NULL, "sb_incr", _("Increment"));
  uiWidgetAddClass (sb->incr, SB_CLASS);
  uiWidgetAlignHorizCenter (sb->incr);
  uiWidgetAlignVertCenter (sb->incr);
  uiWidgetSetAllMargins (sb->incr, 0);
  uiBoxPackEnd (sb->hbox, sb->incr);

  sb->decr = uiCreateButton (sb->sbcbdecr, NULL, "sb_decr", _("Decrement"));
  uiWidgetAddClass (sb->decr, SB_CLASS);
  uiWidgetAlignHorizCenter (sb->decr);
  uiWidgetAlignVertCenter (sb->decr);
  uiWidgetSetAllMargins (sb->decr, 0);
  uiBoxPackEnd (sb->hbox, sb->decr);

  sb->display = disp;
  uiWidgetAddClass (sb->display, SB_CLASS);
  uiWidgetSetAllMargins (sb->display, 0);
  uiWidgetSetMarginStart (sb->display, 4);
  uiWidgetSetMarginEnd (sb->display, 4);
  uiBoxPackStart (sb->hbox, sb->display);
//  uiWidgetExpandHoriz (sb->display);

  uiBoxPackStart (box, sb->hbox);

  return sb;
}

void
uisbFree (uisb_t *sb)
{
  if (sb == NULL) {
    return;
  }

  uiwcontFree (sb->incr);
  uiwcontFree (sb->decr);
  uiwcontFree (sb->hbox);
  callbackFree (sb->sbcbincr);
  callbackFree (sb->sbcbdecr);
  mdfree (sb);
}

void
uisbExpandHoriz (uisb_t *sb)
{
  if (sb == NULL) {
    return;
  }

  uiWidgetExpandHoriz (sb->hbox);
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
uisbSetState (uisb_t *sb, int state)
{
  if (sb == NULL) {
    return;
  }

  uiWidgetSetState (sb->display, state);
  uiWidgetSetState (sb->incr, state);
  uiWidgetSetState (sb->decr, state);
}

void
uisbSizeGroupAdd (uisb_t *sb, uiwcont_t *sg)
{
  if (sb == NULL || sg == NULL) {
    return;
  }

  uiSizeGroupAdd (sg, sb->hbox);
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

  rc = callbackHandlerI (sb->cb, SB_DECREMENT);
  return rc;
}
