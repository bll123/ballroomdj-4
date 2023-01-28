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

/* as a side effect, hbox is set, and */
/* uiwidget is set to the accent color box (needed by bdj4starterui) */
void
uiutilsAddAccentColorDisplay (UIWidget *vboxp, UIWidget *hboxp, UIWidget *uiwidgetp)
{
  uiCreateHorizBox (hboxp);
  uiBoxPackStart (vboxp, hboxp);

  /* black large square "\xE2\xAC\x9B" */
  /* black square 0xE2 0x96 0xA0 */
  /* black square centered 0xE2 0xAF 0x80 */
  uiCreateLabel (uiwidgetp, "\xE2\xAC\x9B");
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

