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
#include <assert.h>
#include <math.h>

#include "bdjopt.h"
#include "ui.h"
#include "uiutils.h"
#include "sysvars.h"

/* as a side effect, hbox is set, and */
/* uiwidget is set to the accent color box (needed by bdj4starterui) */
void
uiutilsAddAccentColorDisplay (UIWidget *vboxp, UIWidget *hboxp, UIWidget *uiwidgetp)
{
  uiCreateHorizBox (hboxp);
  uiBoxPackStart (vboxp, hboxp);

  /* right half block 0xE2 0x96 0x90 */
  /* full block 0xE2 0x96 0x88 */
  uiCreateLabel (uiwidgetp, "\xE2\x96\x90\xE2\x96\x88");
  uiWidgetSetMarginStart (uiwidgetp, 3);
  uiutilsSetAccentColor (uiwidgetp);
  uiBoxPackEnd (hboxp, uiwidgetp);
}

void
uiutilsSetAccentColor (UIWidget *uiwidgetp)
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
