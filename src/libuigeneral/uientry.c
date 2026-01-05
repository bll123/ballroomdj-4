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

void
uiEntrySetValidate (uiwcont_t *uiwidget, const char *label,
    uientryval_t valfunc, void *udata, int valdelay)
{
  uientrybase_t   *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-validate")) {
    return;
  }

  ebase = &uiwidget->uiint.uientrybase;

  ebase->validateFunc = valfunc;
  ebase->label = label;
  ebase->udata = udata;

  if (valfunc != NULL) {
    if (valdelay == UIENTRY_DELAYED) {
      mstimeset (&ebase->validateTimer, UIENTRY_VAL_TIMER);
      ebase->valdelay = true;
    }
    uiEntrySetInternalValidate (uiwidget);
  }
}

int
uiEntryValidate (uiwcont_t *uiwidget, bool forceflag)
{
  int             rc;
  uientrybase_t   *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate")) {
    return UIENTRY_OK;
  }

  ebase = &uiwidget->uiint.uientrybase;

  if (ebase->validateFunc == NULL) {
    return UIENTRY_OK;
  }
  if (forceflag == false &&
      ebase->valdelay &&
      ! mstimeCheck (&ebase->validateTimer)) {
    return UIENTRY_OK;
  }
  if (forceflag == false &&
      uiEntryChanged (uiwidget) == false) {
    return UIENTRY_OK;
  }

  uiEntryClearChanged (uiwidget);
  mstimeset (&ebase->validateTimer, TM_TIMER_OFF);
  rc = ebase->validateFunc (uiwidget, ebase->label, ebase->udata);
  if (rc == UIENTRY_RESET) {
    mstimeset (&ebase->validateTimer, UIENTRY_VAL_TIMER);
  }
  if (rc == UIENTRY_ERROR) {
    uiEntrySetIcon (uiwidget, "dialog-error");
    ebase->valid = false;
  }
  if (rc == UIENTRY_OK) {
    uiEntryClearIcon (uiwidget);
    ebase->valid = true;
  }
  if (ebase->valdelay) {
    mstimeset (&ebase->validateTimer, UIENTRY_VAL_TIMER);
  }
  return rc;
}

void
uiEntryValidateClear (uiwcont_t *uiwidget)
{
  uientrybase_t   *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate-clear")) {
    return;
  }

  ebase = &uiwidget->uiint.uientrybase;

  if (ebase->validateFunc != NULL) {
    mstimeset (&ebase->validateTimer, TM_TIMER_OFF);
    /* validate-clear is called when the entry is switched to a new value */
    /* assume validity */
    ebase->valid = true;
  }
}

int
uiEntryValidateDir (uiwcont_t *uiwidget, const char *label, void *udata)
{
  int             rc;
  const char      *dir;
  char            tbuff [BDJ4_PATH_MAX];
  uientrybase_t   *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate-dir")) {
    return UIENTRY_OK;
  }

  ebase = &uiwidget->uiint.uientrybase;

  rc = UIENTRY_ERROR;
  dir = uiEntryGetValue (uiwidget);
  ebase->valid = false;
  if (dir != NULL) {
    stpecpy (tbuff, tbuff + sizeof (tbuff), dir);
    pathNormalizePath (tbuff, sizeof (tbuff));
    if (fileopIsDirectory (tbuff)) {
      rc = UIENTRY_OK;
      ebase->valid = true;
    } /* exists */
  } /* not null */

  return rc;
}

int
uiEntryValidateFile (uiwcont_t *uiwidget, const char *label, void *udata)
{
  int             rc;
  const char      *fn;
  char            tbuff [BDJ4_PATH_MAX];
  uientrybase_t   *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate-file")) {
    return UIENTRY_OK;
  }

  ebase = &uiwidget->uiint.uientrybase;

  rc = UIENTRY_ERROR;
  fn = uiEntryGetValue (uiwidget);
  ebase->valid = false;
  if (fn != NULL) {
    if (*fn == '\0') {
      rc = UIENTRY_OK;
      ebase->valid = true;
    } else {
      stpecpy (tbuff, tbuff + sizeof (tbuff), fn);
      pathNormalizePath (tbuff, sizeof (tbuff));
      if (fileopFileExists (tbuff)) {
        rc = UIENTRY_OK;
        ebase->valid = true;
      } /* exists */
    } /* not empty */
  } /* not null */

  return rc;
}

bool
uiEntryIsNotValid (uiwcont_t *uiwidget)
{
  uientrybase_t   *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-not-valid")) {
    return false;
  }

  ebase = &uiwidget->uiint.uientrybase;
  return ! ebase->valid;
}

