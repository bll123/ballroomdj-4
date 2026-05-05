/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bdjopt.h"
#include "uiutils.h"

#include "ui/uiui.h"      // for uisetup_t

void
uiutilsInitSetup (uisetup_t *uisetup)
{
  if (uisetup == NULL) {
    return;
  }

  uisetup->uifont = bdjoptGetCurrentFont ();
  uisetup->listingfont = bdjoptGetListingFont ();
  uisetup->accentColor = bdjoptGetStr (OPT_P_UI_ACCENT_COL);
  uisetup->errorColor = bdjoptGetStr (OPT_P_UI_ERROR_COL);
  uisetup->markColor = bdjoptGetStr (OPT_P_UI_MARK_COL);
  uisetup->rowselColor = bdjoptGetStr (OPT_P_UI_ROWSEL_COL);
  uisetup->rowhlColor = bdjoptGetStr (OPT_P_UI_ROW_HL_COL);
  uisetup->mqbgColor = bdjoptGetStr (OPT_P_MQ_BG_COL);
  uisetup->changedColor = "#11ff11";
  uisetup->is_dark = bdjoptGetNum (OPT_M_UI_DARK);
}
