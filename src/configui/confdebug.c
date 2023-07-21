/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
confuiBuildUIDebug (confuigui_t *gui)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *szgrp;
  nlistidx_t    val;

  logProcBegin (LOG_PROC, "confuiBuildUIDebug");
  vbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();

  /* debug */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: debug options that can be turned on and off */
      _("Debug"), CONFUI_ID_NONE);

  val = bdjoptGetNum (OPT_G_DEBUGLVL);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwcontFree (vbox);
  vbox = uiCreateVertBox ();
  uiBoxPackStart (hbox, vbox);

  confuiMakeItemCheckButton (gui, vbox, szgrp, "Important",
      CONFUI_WIDGET_DEBUG_1, -1, (val & 1));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Basic",
      CONFUI_WIDGET_DEBUG_2, -1, (val & 2));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Messages",
      CONFUI_WIDGET_DEBUG_4, -1, (val & 4));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Informational",
      CONFUI_WIDGET_DEBUG_8, -1, (val & 8));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Actions",
      CONFUI_WIDGET_DEBUG_16, -1, (val & 16));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Apply Adjustments",
      CONFUI_WIDGET_DEBUG_2097152, -1, (val & 2097152));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Audio Tags",
      CONFUI_WIDGET_DEBUG_4194304, -1, (val & 4194304));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Dance Selection",
      CONFUI_WIDGET_DEBUG_128, -1, (val & 128));

  uiwcontFree (vbox);
  vbox = uiCreateVertBox ();
  uiBoxPackStart (hbox, vbox);

  confuiMakeItemCheckButton (gui, vbox, szgrp, "Database",
      CONFUI_WIDGET_DEBUG_1024, -1, (val & 1024));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Database Update",
      CONFUI_WIDGET_DEBUG_262144, -1, (val & 262144));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Datafile",
      CONFUI_WIDGET_DEBUG_16384, -1, (val & 16384));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "iTunes Parse",
      CONFUI_WIDGET_DEBUG_1048576, -1, (val & 1048576));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "List",
      CONFUI_WIDGET_DEBUG_32, -1, (val & 32));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Player",
      CONFUI_WIDGET_DEBUG_8192, -1, (val & 8192));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Procedures",
      CONFUI_WIDGET_DEBUG_4096, -1, (val & 4096));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Process",
      CONFUI_WIDGET_DEBUG_32768, -1, (val & 32768));

  uiwcontFree (vbox);
  vbox = uiCreateVertBox ();
  uiBoxPackStart (hbox, vbox);

  confuiMakeItemCheckButton (gui, vbox, szgrp, "Program State",
      CONFUI_WIDGET_DEBUG_524288, -1, (val & 524288));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Random Access File",
      CONFUI_WIDGET_DEBUG_2048, -1, (val & 2048));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Socket",
      CONFUI_WIDGET_DEBUG_512, -1, (val & 512));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Song Selection",
      CONFUI_WIDGET_DEBUG_64, -1, (val & 64));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Volume",
      CONFUI_WIDGET_DEBUG_256, -1, (val & 256));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Web Client",
      CONFUI_WIDGET_DEBUG_131072, -1, (val & 131072));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Web Server",
      CONFUI_WIDGET_DEBUG_65536, -1, (val & 65536));

  for (int i = CONFUI_WIDGET_DEBUG_1; i < CONFUI_WIDGET_DEBUG_MAX; ++i) {
    long    idx;
    long    bitval;

    gui->uiitem [i].outtype = CONFUI_OUT_DEBUG;
    idx = i - CONFUI_WIDGET_DEBUG_1;
    bitval = 1 << idx;
    gui->uiitem [i].debuglvl = bitval;
  }

  uiwcontFree (vbox);
  uiwcontFree (hbox);
  uiwcontFree (szgrp);

  logProcEnd (LOG_PROC, "confuiBuildUIDebug", "");
}

