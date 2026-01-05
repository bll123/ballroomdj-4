/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "callback.h"
#include "fileop.h"
#include "log.h"
#include "pathutil.h"
#include "tmutil.h"
#include "uigeneral.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uientry.h"

static void uiEntryValidateStart (uiwcont_t *uiwidget);

void
uiEntryValidateHandler (uiwcont_t *uiwidget)
{
  int           rc;
  uientrybase_t *ebase;

  ebase = &uiwidget->uiint.uientrybase;

  if (ebase->valdelay) {
    uiEntryValidateStart (uiwidget);
  } else {
    if (ebase->validateFunc != NULL) {
      rc = ebase->validateFunc (uiwidget, ebase->label, ebase->udata);
      if (rc == UIENTRY_ERROR) {
        ebase->valid = false;
      }
      if (rc == UIENTRY_OK) {
        ebase->valid = true;
      }
    }
  }
  return;
}

/* internal routines */

static void
uiEntryValidateStart (uiwcont_t *uiwidget)
{
  uientrybase_t   *ebase;

  ebase = &uiwidget->uiint.uientrybase;
  if (ebase->validateFunc != NULL) {
    mstimeset (&ebase->validateTimer, UIENTRY_VAL_TIMER);
  }
}

