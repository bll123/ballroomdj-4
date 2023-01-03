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
#include "ui.h"

void
confuiBuildUIDebug (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      sg;
  nlistidx_t    val;

  logProcBegin (LOG_PROC, "confuiBuildUIDebug");
  uiCreateVertBox (&vbox);

  /* debug */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: debug options that can be turned on and off */
      _("Debug"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateVertBox (&vbox);
  uiBoxPackStart (&hbox, &vbox);

  val = bdjoptGetNum (OPT_G_DEBUGLVL);
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Important",
      CONFUI_WIDGET_DEBUG_1, -1,
      (val & 1));
  gui->uiitem [CONFUI_WIDGET_DEBUG_1].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Basic",
      CONFUI_WIDGET_DEBUG_2, -1,
      (val & 2));
  gui->uiitem [CONFUI_WIDGET_DEBUG_2].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Messages",
      CONFUI_WIDGET_DEBUG_4, -1,
      (val & 4));
  gui->uiitem [CONFUI_WIDGET_DEBUG_4].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Main",
      CONFUI_WIDGET_DEBUG_8, -1,
      (val & 8));
  gui->uiitem [CONFUI_WIDGET_DEBUG_8].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Actions",
      CONFUI_WIDGET_DEBUG_16, -1,
      (val & 16));
  gui->uiitem [CONFUI_WIDGET_DEBUG_16].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "List",
      CONFUI_WIDGET_DEBUG_32, -1,
      (val & 32));
  gui->uiitem [CONFUI_WIDGET_DEBUG_32].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Song Selection",
      CONFUI_WIDGET_DEBUG_64, -1,
      (val & 64));
  gui->uiitem [CONFUI_WIDGET_DEBUG_64].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Dance Selection",
      CONFUI_WIDGET_DEBUG_128, -1,
      (val & 128));
  gui->uiitem [CONFUI_WIDGET_DEBUG_128].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Volume",
      CONFUI_WIDGET_DEBUG_256, -1,
      (val & 256));
  gui->uiitem [CONFUI_WIDGET_DEBUG_256].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Socket",
      CONFUI_WIDGET_DEBUG_512, -1,
      (val & 512));
  gui->uiitem [CONFUI_WIDGET_DEBUG_512].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Database",
      CONFUI_WIDGET_DEBUG_1024, -1,
      (val & 1024));
  gui->uiitem [CONFUI_WIDGET_DEBUG_1024].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Random Access File",
      CONFUI_WIDGET_DEBUG_2048, -1,
      (val & 2048));

  uiCreateVertBox (&vbox);
  uiBoxPackStart (&hbox, &vbox);

  uiCreateSizeGroupHoriz (&sg);

  gui->uiitem [CONFUI_WIDGET_DEBUG_2048].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Procedures",
      CONFUI_WIDGET_DEBUG_4096, -1,
      (val & 4096));
  gui->uiitem [CONFUI_WIDGET_DEBUG_4096].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Player",
      CONFUI_WIDGET_DEBUG_8192, -1,
      (val & 8192));
  gui->uiitem [CONFUI_WIDGET_DEBUG_8192].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Datafile",
      CONFUI_WIDGET_DEBUG_16384, -1,
      (val & 16384));
  gui->uiitem [CONFUI_WIDGET_DEBUG_16384].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Process",
      CONFUI_WIDGET_DEBUG_32768, -1,
      (val & 32768));
  gui->uiitem [CONFUI_WIDGET_DEBUG_32768].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Web Server",
      CONFUI_WIDGET_DEBUG_65536, -1,
      (val & 65536));
  gui->uiitem [CONFUI_WIDGET_DEBUG_65536].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Web Client",
      CONFUI_WIDGET_DEBUG_131072, -1,
      (val & 131072));
  gui->uiitem [CONFUI_WIDGET_DEBUG_131072].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Database Update",
      CONFUI_WIDGET_DEBUG_262144, -1,
      (val & 262144));
  gui->uiitem [CONFUI_WIDGET_DEBUG_262144].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "Program State",
      CONFUI_WIDGET_DEBUG_524288, -1,
      (val & 524288));
  gui->uiitem [CONFUI_WIDGET_DEBUG_524288].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (gui, &vbox, &sg, "iTunes Parse",
      CONFUI_WIDGET_DEBUG_1048576, -1,
      (val & 1048576));
  gui->uiitem [CONFUI_WIDGET_DEBUG_1048576].outtype = CONFUI_OUT_DEBUG;
  logProcEnd (LOG_PROC, "confuiBuildUIDebug", "");
}

