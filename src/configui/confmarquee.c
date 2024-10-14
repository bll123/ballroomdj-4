/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
#include "ui.h"

void
confuiInitMarquee (confuigui_t *gui)
{
  confuiSpinboxTextInitDataNum (gui, "cu-marquee-show",
      CONFUI_SPINBOX_MARQUEE_SHOW,
      /* CONTEXT: configuration: show-marquee: off */
      MARQUEE_SHOW_OFF, _("Off"),
      /* CONTEXT: configuration: show-marquee: minimize */
      MARQUEE_SHOW_MINIMIZE, _("Minimised"),
      /* CONTEXT: configuration: show-marquee: visible */
      MARQUEE_SHOW_VISIBLE, _("Visible"),
      -1);
}

void
confuiBuildUIMarquee (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *szgrp;

  logProcBegin ();
  vbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();

  /* marquee */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: options associated with the marquee (suggested: song display) */
      _("Marquee"), CONFUI_ID_NONE);

  /* CONTEXT: configuration: show-marquee: selection */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Show Marquee"),
      CONFUI_SPINBOX_MARQUEE_SHOW, OPT_P_MARQUEE_SHOW,
      CONFUI_OUT_NUM, bdjoptGetNum (OPT_P_MARQUEE_SHOW), NULL);

  /* CONTEXT: configuration: The theme to use for the marquee display */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Marquee Theme"),
      CONFUI_SPINBOX_MQ_THEME, OPT_M_MQ_THEME,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_MQ_THEME].listidx, NULL);

  /* CONTEXT: configuration: The font to use for the marquee display */
  confuiMakeItemFontButton (gui, vbox, szgrp, _("Marquee Font"),
      CONFUI_WIDGET_MQ_FONT, OPT_M_MQ_FONT,
      bdjoptGetStr (OPT_M_MQ_FONT));

  /* CONTEXT: (noun) configuration: the length of the queue displayed on the marquee */
  confuiMakeItemSpinboxNum (gui, vbox, szgrp, NULL, _("Queue Length"),
      CONFUI_WIDGET_MQ_QUEUE_LEN, OPT_P_MQQLEN,
      1, 20, bdjoptGetNum (OPT_P_MQQLEN), NULL);

  /* CONTEXT: configuration: marquee: show the song information (artist/title) on the marquee */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Show Song Information"),
      CONFUI_SWITCH_MQ_SHOW_SONG_INFO, OPT_P_MQ_SHOW_INFO,
      bdjoptGetNum (OPT_P_MQ_SHOW_INFO), NULL, CONFUI_NO_INDENT);

  /* CONTEXT: configuration: marquee: the accent color used for the marquee */
  confuiMakeItemColorButton (gui, vbox, szgrp, _("Accent Colour"),
      CONFUI_WIDGET_MQ_ACCENT_COLOR, OPT_P_MQ_ACCENT_COL,
      bdjoptGetStr (OPT_P_MQ_ACCENT_COL));

  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  logProcEnd ("");
}

