/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjopt.h"
#include "sysvars.h"
#include "ui.h"
#include "uiutils.h"
#include "validate.h"

/* as a side effect, hbox is set, and */
/* uiwidget is set to the accent color box (needed by bdj4starterui) */
void
uiutilsAddAccentColorDisplay (uiwcont_t *vboxp, uiutilsaccent_t *accent)
{
  uiwcont_t       *hbox;
  uiwcont_t       *label;

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxp, hbox);

  /* right half block 0xE2 0x96 0x90 */
  /* full block 0xE2 0x96 0x88 */
  label = uiCreateLabel ("\xE2\x96\x90\xE2\x96\x88");
  uiWidgetSetMarginStart (label, 3);
  uiutilsSetAccentColor (label);
  uiBoxPackEnd (hbox, label);

  accent->hbox = hbox;
  accent->label = label;
}

void
uiutilsSetAccentColor (uiwcont_t *uiwidgetp)
{
  char      classnm [100];

  snprintf (classnm, sizeof (classnm), "profcol%s",
      bdjoptGetStr (OPT_P_UI_PROFILE_COL) + 1);
  uiLabelAddClass (classnm, bdjoptGetStr (OPT_P_UI_PROFILE_COL));
  uiWidgetSetClass (uiwidgetp, classnm);
}

const char *
uiutilsGetCurrentFont (void)
{
  const char  *tstr;

  tstr = bdjoptGetStr (OPT_MP_UIFONT);
  if (tstr == NULL || ! *tstr) {
    tstr = sysvarsGetStr (SV_FONT_DEFAULT);
  }
  return tstr;
}

int
uiutilsValidateSongListName (uientry_t *entry, void *udata)
{
  uiwcont_t   *statusMsg = udata;
  int         rc;
  const char  *str;
  char        tbuff [200];
  const char  *valstr;

  rc = UIENTRY_OK;
  if (statusMsg != NULL) {
    uiLabelSetText (statusMsg, "");
  }
  str = uiEntryGetValue (entry);
  valstr = validate (str, VAL_NOT_EMPTY | VAL_NO_SLASHES);
  if (valstr != NULL) {
    if (statusMsg != NULL) {
      snprintf (tbuff, sizeof (tbuff), valstr, str);
      uiLabelSetText (statusMsg, tbuff);
    }
    rc = UIENTRY_ERROR;
  }

  return rc;
}


void
uiutilsProgressStatus (uiwcont_t *statusMsg, int count, int tot)
{
  char        tbuff [200];

  if (statusMsg == NULL) {
    return;
  }

  if (count < 0 && tot < 0) {
    uiLabelSetText (statusMsg, "");
  } else {
    /* CONTEXT: please wait... (count/total) status message */
    snprintf (tbuff, sizeof (tbuff), _("Please wait\xe2\x80\xa6 (%1$d/%2$d)"), count, tot);
    uiLabelSetText (statusMsg, tbuff);
  }

  return;
}

