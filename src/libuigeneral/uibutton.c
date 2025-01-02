/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "log.h"
#include "tmutil.h"
#include "uigeneral.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibutton.h"

bool
uiButtonCheckRepeat (uiwcont_t *uiwidget)
{
  uibuttonbase_t  *bbase;
  bool            rc = false;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return rc;
  }

  bbase = &uiwidget->uiint.uibuttonbase;

  if (bbase->repeating) {
    if (mstimeCheck (&bbase->repeatTimer)) {
      if (bbase->cb != NULL) {
        bbase->repeating = false;
        callbackHandler (bbase->cb);
        mstimeset (&bbase->repeatTimer, bbase->repeatMS);
        bbase->repeating = true;
      }
    }
    rc = true;
  }

  return rc;
}

bool
uiButtonPressCallback (void *udata)
{
  uiwcont_t       *uiwidget = udata;
  uibuttonbase_t  *bbase;
  callback_t      *uicb;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-state")) {
    return UICB_CONT;
  }

  bbase = &uiwidget->uiint.uibuttonbase;
  uicb = bbase->cb;

  bbase->repeating = false;
  uicb = bbase->cb;
  if (uicb == NULL) {
    return UICB_CONT;
  }
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: button-press");
  if (uicb != NULL) {
    callbackHandler (uicb);
  }
  /* the first time the button is pressed, the repeat timer should have */
  /* a longer initial delay. */
  mstimeset (&bbase->repeatTimer, bbase->repeatMS * 3);
  bbase->repeating = true;
  return UICB_CONT;
}

bool
uiButtonReleaseCallback (void *udata)
{
  uiwcont_t       *uiwidget = udata;
  uibuttonbase_t  *bbase;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-state")) {
    return UICB_CONT;
  }

  bbase = &uiwidget->uiint.uibuttonbase;

  bbase->repeating = false;
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: button-release");
  return UICB_CONT;
}
