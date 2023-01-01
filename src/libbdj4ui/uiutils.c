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

  uiCreateLabel (uiwidgetp, "");
  uiWidgetSetSizeRequest (uiwidgetp, 25, 25);
  uiWidgetSetMarginStart (uiwidgetp, 3);
  uiLabelSetBackgroundColor (uiwidgetp, bdjoptGetStr (OPT_P_UI_PROFILE_COL));
  uiBoxPackEnd (hboxp, uiwidgetp);
}
