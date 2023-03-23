/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "configui.h"
#include "log.h"
#include "sysvars.h"
#include "ui.h"

void
confuiBuildUIUserInterface (confuigui_t *gui)
{
  uiwcont_t     *vbox;
  uiwcont_t     *szgrp;
  char          *tstr;

  logProcBegin (LOG_PROC, "confuiBuildUIUserInterface");
  vbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();

  /* user interface */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: options associated with the user interface */
      _("User Interface"), CONFUI_ID_NONE);

  /* CONTEXT: configuration: the theme to use for the user interface */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Theme"),
      CONFUI_SPINBOX_UI_THEME, OPT_MP_UI_THEME, CONFUI_OUT_STR,
      gui->uiitem [CONFUI_SPINBOX_UI_THEME].listidx, NULL);

  tstr = bdjoptGetStr (OPT_MP_UIFONT);
  if (! *tstr) {
    tstr = sysvarsGetStr (SV_FONT_DEFAULT);
  }
  /* CONTEXT: configuration: the font to use for the user interface */
  confuiMakeItemFontButton (gui, vbox, szgrp, _("Font"),
      CONFUI_WIDGET_UI_FONT, OPT_MP_UIFONT, tstr);

  tstr = bdjoptGetStr (OPT_MP_LISTING_FONT);
  if (! *tstr) {
    tstr = sysvarsGetStr (SV_FONT_DEFAULT);
  }
  /* CONTEXT: configuration: the font to use for the queues and song lists */
  confuiMakeItemFontButton (gui, vbox, szgrp, _("Listing Font"),
      CONFUI_WIDGET_UI_LISTING_FONT, OPT_MP_LISTING_FONT, tstr);

  /* CONTEXT: configuration: scaling factor for the user interface */
  confuiMakeItemSpinboxNum (gui, vbox, szgrp, NULL, _("Scale"),
      CONFUI_WIDGET_UI_SCALE, OPT_M_SCALE,
      1, 4, bdjoptGetNum (OPT_M_SCALE), NULL);

  /* CONTEXT: configuration: the accent color to use for the user interface */
  confuiMakeItemColorButton (gui, vbox, szgrp, _("Accent Colour"),
      CONFUI_WIDGET_UI_ACCENT_COLOR, OPT_P_UI_ACCENT_COL,
      bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  logProcEnd (LOG_PROC, "confuiBuildUIUserInterface", "");

  uiwcontFree (vbox);
  uiwcontFree (szgrp);
}

