/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

  logProcBegin ();
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
      CONFUI_DBG_IMPORTANT, -1, (val & LOG_IMPORTANT));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Basic",
      CONFUI_DBG_BASIC, -1, (val & LOG_BASIC));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Messages",
      CONFUI_DBG_MSGS, -1, (val & LOG_MSGS));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Informational",
      CONFUI_DBG_INFO, -1, (val & LOG_INFO));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Actions",
      CONFUI_DBG_ACTIONS, -1, (val & LOG_ACTIONS));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Audio Adjust",
      CONFUI_DBG_AUDIO_ADJUST, -1, (val & LOG_AUDIO_ADJUST));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Audio ID",
      CONFUI_DBG_AUDIO_ID, -1, (val & LOG_AUDIO_ID));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Audio ID Dump",
      CONFUI_DBG_AUDIOID_DUMP, -1, (val & LOG_AUDIOID_DUMP));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Audio Tags",
      CONFUI_DBG_AUDIO_TAG, -1, (val & LOG_AUDIO_TAG));

  uiwcontFree (vbox);
  vbox = uiCreateVertBox ();
  uiBoxPackStart (hbox, vbox);

  confuiMakeItemCheckButton (gui, vbox, szgrp, "Dance Selection",
      CONFUI_DBG_DANCESEL, -1, (val & LOG_DANCESEL));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Database",
      CONFUI_DBG_DB, -1, (val & LOG_DB));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Database Update",
      CONFUI_DBG_DBUPDATE, -1, (val & LOG_DBUPDATE));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Datafile",
      CONFUI_DBG_DATAFILE, -1, (val & LOG_DATAFILE));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Grouping",
      CONFUI_DBG_GROUPING, -1, (val & LOG_GROUPING));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "iTunes",
      CONFUI_DBG_ITUNES, -1, (val & LOG_ITUNES));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "List",
      CONFUI_DBG_LIST, -1, (val & LOG_LIST));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Player",
      CONFUI_DBG_PLAYER, -1, (val & LOG_PLAYER));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Procedures",
      CONFUI_DBG_PROC, -1, (val & LOG_PROC));

  uiwcontFree (vbox);
  vbox = uiCreateVertBox ();
  uiBoxPackStart (hbox, vbox);

  confuiMakeItemCheckButton (gui, vbox, szgrp, "Process",
      CONFUI_DBG_PROCESS, -1, (val & LOG_PROCESS));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Program State",
      CONFUI_DBG_PROGSTATE, -1, (val & LOG_PROGSTATE));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Random Access File",
      CONFUI_DBG_RAFILE, -1, (val & LOG_RAFILE));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Socket",
      CONFUI_DBG_SOCKET, -1, (val & LOG_SOCKET));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Song Selection",
      CONFUI_DBG_SONGSEL, -1, (val & LOG_SONGSEL));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Virtual List",
      CONFUI_DBG_VIRTLIST, -1, (val & LOG_VIRTLIST));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Volume",
      CONFUI_DBG_VOLUME, -1, (val & LOG_VOLUME));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Web Client",
      CONFUI_DBG_WEBCLIENT, -1, (val & LOG_WEBCLIENT));
  confuiMakeItemCheckButton (gui, vbox, szgrp, "Web Server",
      CONFUI_DBG_WEBSRV, -1, (val & LOG_WEBSRV));

  for (int i = CONFUI_DBG_IMPORTANT; i < CONFUI_DBG_MAX; ++i) {
    long    idx;
    long    bitval;

    gui->uiitem [i].outtype = CONFUI_OUT_DEBUG;
    idx = i - CONFUI_DBG_IMPORTANT;
    bitval = 1 << idx;
    gui->uiitem [i].debuglvl = bitval;
  }

  uiwcontFree (vbox);
  uiwcontFree (hbox);
  uiwcontFree (szgrp);

  logProcEnd ("");
}

