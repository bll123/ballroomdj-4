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
#include "mdebug.h"
#include "pathdisp.h"
#include "tmutil.h"
#include "ui.h"

uiwcont_t *
uiEntryInit  (int entrySize, int maxSize)
{
  uientrygeneric_t  *ebase;
  uiwcont_t     *uientry;

  uientry = uiEntryInit_r (entrySize, maxSize);

  ebase = mdmalloc (sizeof (uientrygeneric_t));
  uientry->uientrygen = ebase;
  ebase->validateFunc = NULL;
  ebase->label = "";
  ebase->udata = NULL;
  mstimeset (&ebase->validateTimer, TM_TIMER_OFF);
  ebase->valdelay = false;
  ebase->valid = true;

  return uientry;
}

void
uiEntryFree (uiwcont_t *uientry)
{
  uientrygeneric_t  *ebase;

  ebase = uientry->uientrygen;
  mdfree (ebase);
  uiEntryFree_r (uientry);
}

void
uiEntrySetValidate (uiwcont_t *uiwidget, const char *label,
    uientryval_t valfunc, void *udata, int valdelay)
{
  uientrygeneric_t   *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-validate")) {
    return;
  }

  ebase = uiwidget->uientrygen;

  ebase->validateFunc = valfunc;
  ebase->label = label;
  ebase->udata = udata;
  ebase->useicon = true;

  if (valfunc != NULL) {
    if (valdelay == UIENTRY_DELAYED || valdelay == UIENTRY_DELAY_NO_ICON) {
      ebase->valdelaytime = UIENTRY_VAL_TIMER;
      if (valdelay == UIENTRY_DELAY_NO_ICON) {
        ebase->valdelaytime = UIENTRY_VAL_TIMER * 2;
        ebase->useicon = false;
      }
      mstimeset (&ebase->validateTimer, ebase->valdelaytime);
      ebase->valdelay = true;
    }
    uiEntrySetInternalValidate (uiwidget);
  }
}

int
uiEntryValidate (uiwcont_t *uiwidget, bool forceflag)
{
  int             rc;
  uientrygeneric_t    *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate")) {
    return UIENTRY_OK;
  }

  ebase = uiwidget->uientrygen;

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
    mstimeset (&ebase->validateTimer, ebase->valdelaytime);
  }
  if (rc == UIENTRY_ERROR && ebase->useicon) {
    uiEntrySetIcon (uiwidget, "dialog-error");
    ebase->valid = false;
  }
  if (rc == UIENTRY_OK) {
    uiEntryClearIcon (uiwidget);
    ebase->valid = true;
  }
  if (ebase->valdelay) {
    mstimeset (&ebase->validateTimer, ebase->valdelaytime);
  }
  return rc;
}

void
uiEntryValidateClear (uiwcont_t *uiwidget)
{
  uientrygeneric_t   *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate-clear")) {
    return;
  }

  ebase = uiwidget->uientrygen;

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
  uientrygeneric_t    *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate-dir")) {
    return UIENTRY_OK;
  }

  ebase = uiwidget->uientrygen;

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
  uientrygeneric_t    *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate-file")) {
    return UIENTRY_OK;
  }

  ebase = uiwidget->uientrygen;

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
  uientrygeneric_t   *ebase;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-not-valid")) {
    return false;
  }

  ebase = uiwidget->uientrygen;
  return ! ebase->valid;
}

