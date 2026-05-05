/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "tmutil.h"
#include "ui.h"

uiwcont_t *
uiCreateButton (callback_t *uicb, const char *title, const char *imagenm, const char *tooltiptxt)
{
  uiwcont_t       *uiwidget;
  uibuttongeneric_t   *bgen;

  uiwidget = uiCreateButton_r (uicb, title, imagenm, tooltiptxt);

  bgen = mdmalloc (sizeof (uibuttongeneric_t));
  uiwidget->uibuttongen = bgen;
  bgen->cb = uicb;
  bgen->presscb = callbackInit (uiButtonPressCallback,
      uiwidget, "button-repeat-press");
  bgen->releasecb = callbackInit (uiButtonReleaseCallback,
      uiwidget, "button-repeat-release");
  bgen->repeating = false;
  bgen->repeatOn = false;
  bgen->repeatMS = 250;
  bgen->state = BUTTON_OFF;

  if (imagenm != NULL) {
    bgen->state = BUTTON_ON;    // force set of image
    uiButtonSetState (uiwidget, BUTTON_OFF);
  }

  return uiwidget;
}

void
uiButtonFree (uiwcont_t *uiwidget)
{
  uibuttongeneric_t  *bgen;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return;
  }

  bgen = uiwidget->uibuttongen;
  callbackFree (bgen->presscb);
  callbackFree (bgen->releasecb);
  mdfree (bgen);
}

bool
uiButtonCheckRepeat (uiwcont_t *uiwidget)
{
  uibuttongeneric_t  *bgen;
  bool            rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return rc;
  }

  bgen = uiwidget->uibuttongen;

  if (bgen->repeating) {
    if (mstimeCheck (&bgen->repeatTimer)) {
      if (bgen->cb != NULL) {
        bgen->repeating = false;
        callbackHandler (bgen->cb);
        mstimeset (&bgen->repeatTimer, bgen->repeatMS);
        bgen->repeating = true;
      }
    }
    rc = true;
  }

  return rc;
}

bool
uiButtonPressCallback (void *udata)
{
  uiwcont_t         *uiwidget = udata;
  uibuttongeneric_t *bgen;
  callback_t        *uicb;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-state")) {
    return UICB_CONT;
  }

  bgen = uiwidget->uibuttongen;
  uicb = bgen->cb;

  bgen->repeating = false;
  uicb = bgen->cb;
  if (uicb == NULL) {
    return UICB_CONT;
  }
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: button-press");
  if (uicb != NULL) {
    callbackHandler (uicb);
  }
  /* the first time the button is pressed, the repeat timer should have */
  /* a longer initial delay. */
  mstimeset (&bgen->repeatTimer, bgen->repeatMS * 3);
  bgen->repeating = true;
  return UICB_CONT;
}

bool
uiButtonReleaseCallback (void *udata)
{
  uiwcont_t       *uiwidget = udata;
  uibuttongeneric_t  *bgen;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-release-cb")) {
    return UICB_CONT;
  }

  bgen = uiwidget->uibuttongen;

  bgen->repeating = false;
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: button-release");
  return UICB_CONT;
}

uibuttonstate_t
uiButtonGetState (uiwcont_t *uiwidget)
{
  uibuttongeneric_t *bgen;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return BUTTON_OFF;
  }

  bgen = uiwidget->uibuttongen;
  if (bgen == NULL) {
    return BUTTON_OFF;
  }

  return bgen->state;
}

