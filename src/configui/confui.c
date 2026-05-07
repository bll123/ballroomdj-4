/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
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
#include "tmutil.h"
#include "ui.h"

void
confuiBuildUIUserInterface (confuigui_t *gui)
{
  uiwcont_t     *grid;
  const char    *tstr;

  grid = uiCreateGrid ();

  logMsg (LOG_DBG, LOG_INFO, "conf: build-ui: ui");
  /* user interface */
  confuiMakeNotebookTab (grid, gui,
      /* CONTEXT: configuration: options associated with the user interface */
      _("User Interface"), CONFUI_ID_NONE);

//fprintf (stderr, "c-ui: theme\n");
  /* CONTEXT: configuration: the theme to use for the user interface */
  confuiMakeItemDropdownG (gui, grid, _("Theme"),
      CONFUI_DD_UI_THEME, OPT_M_UI_THEME, confuiUIThemeSelect);

//fprintf (stderr, "c-ui: font\n");
  tstr = bdjoptGetStr (OPT_M_UI_FONT);
  if (tstr == NULL || ! *tstr) {
    tstr = sysvarsGetStr (SV_FONT_DEFAULT);
  }
  /* CONTEXT: configuration: the font to use for the user interface */
  confuiMakeItemFontButtonG (gui, grid, _("Font"),
      CONFUI_WIDGET_UI_FONT, OPT_M_UI_FONT, tstr);

//fprintf (stderr, "c-ui: list-font\n");
  tstr = bdjoptGetStr (OPT_M_LISTING_FONT);
  if (tstr == NULL || ! *tstr) {
    tstr = sysvarsGetStr (SV_FONT_DEFAULT);
  }
  /* CONTEXT: configuration: the font to use for the queues and song lists */
  confuiMakeItemFontButtonG (gui, grid, _("Listing Font"),
      CONFUI_WIDGET_UI_LISTING_FONT, OPT_M_LISTING_FONT, tstr);

//fprintf (stderr, "c-ui: scale\n");
  /* CONTEXT: configuration: scaling factor for the user interface */
  confuiMakeItemSpinboxNumG (gui, grid, _("Scale"),
      CONFUI_SB_NUM_UI_SCALE, OPT_M_SCALE,
      1, 4, bdjoptGetNum (OPT_M_SCALE), NULL);

//fprintf (stderr, "c-ui: accent\n");
  /* CONTEXT: configuration: the accent color to use for the user interface */
  confuiMakeItemColorButtonG (gui, grid, _("Accent Colour"),
      CONFUI_WIDGET_UI_ACCENT_COLOR, OPT_P_UI_ACCENT_COL,
      bdjoptGetStr (OPT_P_UI_ACCENT_COL));

//fprintf (stderr, "c-ui: error\n");
  /* CONTEXT: configuration: the error color to use for the user interface */
  confuiMakeItemColorButtonG (gui, grid, _("Error Colour"),
      CONFUI_WIDGET_UI_ERROR_COLOR, OPT_P_UI_ERROR_COL,
      bdjoptGetStr (OPT_P_UI_ERROR_COL));

//fprintf (stderr, "c-ui: song-mark\n");
  /* CONTEXT: configuration: user interface: the song mark color for songs in the song list */
  confuiMakeItemColorButtonG (gui, grid, _("Song Mark Colour"),
      CONFUI_WIDGET_UI_MARK_COLOR, OPT_P_UI_MARK_COL,
      bdjoptGetStr (OPT_P_UI_MARK_COL));

  uiGridPostProcess (grid);
}

