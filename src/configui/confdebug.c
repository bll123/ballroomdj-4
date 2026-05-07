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
#include "ui.h"

void
confuiBuildUIDebug (confuigui_t *gui)
{
  uiwcont_t     *grid;
  nlistidx_t    val;


  logMsg (LOG_DBG, LOG_INFO, "conf: build-ui: debug");
  grid = uiCreateGrid ();

  /* debug */
  confuiMakeNotebookTab (grid, gui,
      /* CONTEXT: configuration: debug options that can be turned on and off */
      _("Debug"), CONFUI_ID_NONE);

  val = bdjoptGetNum (OPT_G_DEBUGLVL);

  confuiMakeItemCheckButton (gui, grid, "Important",
      CONFUI_DBG_IMPORTANT, -1, (val & LOG_IMPORTANT));
  confuiMakeItemCheckButton (gui, grid, "Basic",
      CONFUI_DBG_BASIC, -1, (val & LOG_BASIC));
  confuiMakeItemCheckButton (gui, grid, "Messages",
      CONFUI_DBG_MSGS, -1, (val & LOG_MSGS));
  confuiMakeItemCheckButton (gui, grid, "Informational",
      CONFUI_DBG_INFO, -1, (val & LOG_INFO));
  confuiMakeItemCheckButton (gui, grid, "Actions",
      CONFUI_DBG_ACTIONS, -1, (val & LOG_ACTIONS));
  confuiMakeItemCheckButton (gui, grid, "Audio Adjust",
      CONFUI_DBG_AUDIO_ADJUST, -1, (val & LOG_AUDIO_ADJUST));
  confuiMakeItemCheckButton (gui, grid, "Audio ID",
      CONFUI_DBG_AUDIO_ID, -1, (val & LOG_AUDIO_ID));
  confuiMakeItemCheckButton (gui, grid, "Audio ID Dump",
      CONFUI_DBG_AUDIOID_DUMP, -1, (val & LOG_AUDIOID_DUMP));
  confuiMakeItemCheckButton (gui, grid, "Audio Source",
      CONFUI_DBG_AUDIOSRC, -1, (val & LOG_AUDIOSRC));
  confuiMakeItemCheckButton (gui, grid, "Audio Tags",
      CONFUI_DBG_AUDIO_TAG, -1, (val & LOG_AUDIO_TAG));

  uiGridNewColumn (grid);
  confuiMakeItemCheckButton (gui, grid, "Dance Selection",
      CONFUI_DBG_DANCESEL, -1, (val & LOG_DANCESEL));
  confuiMakeItemCheckButton (gui, grid, "Database",
      CONFUI_DBG_DB, -1, (val & LOG_DB));
  confuiMakeItemCheckButton (gui, grid, "Database Update",
      CONFUI_DBG_DBUPDATE, -1, (val & LOG_DBUPDATE));
  confuiMakeItemCheckButton (gui, grid, "Datafile",
      CONFUI_DBG_DATAFILE, -1, (val & LOG_DATAFILE));
  confuiMakeItemCheckButton (gui, grid, "Grouping",
      CONFUI_DBG_GROUPING, -1, (val & LOG_GROUPING));
  confuiMakeItemCheckButton (gui, grid, "iTunes",
      CONFUI_DBG_ITUNES, -1, (val & LOG_ITUNES));
  confuiMakeItemCheckButton (gui, grid, "List",
      CONFUI_DBG_LIST, -1, (val & LOG_LIST));
  confuiMakeItemCheckButton (gui, grid, "Player",
      CONFUI_DBG_PLAYER, -1, (val & LOG_PLAYER));
  confuiMakeItemCheckButton (gui, grid, "Procedures",
      CONFUI_DBG_PROC, -1, (val & LOG_PROC));
  confuiMakeItemCheckButton (gui, grid, "Process",
      CONFUI_DBG_PROCESS, -1, (val & LOG_PROCESS));

  uiGridNewColumn (grid);
  confuiMakeItemCheckButton (gui, grid, "Program State",
      CONFUI_DBG_PROGSTATE, -1, (val & LOG_PROGSTATE));
  confuiMakeItemCheckButton (gui, grid, "Random Access File",
      CONFUI_DBG_RAFILE, -1, (val & LOG_RAFILE));
  confuiMakeItemCheckButton (gui, grid, "Socket",
      CONFUI_DBG_SOCKET, -1, (val & LOG_SOCKET));
  confuiMakeItemCheckButton (gui, grid, "Song Selection",
      CONFUI_DBG_SONGSEL, -1, (val & LOG_SONGSEL));
  confuiMakeItemCheckButton (gui, grid, "User Interface",
      CONFUI_DBG_UI, -1, (val & LOG_UI));
  confuiMakeItemCheckButton (gui, grid, "Virtual List",
      CONFUI_DBG_VIRTLIST, -1, (val & LOG_VIRTLIST));
  confuiMakeItemCheckButton (gui, grid, "Volume",
      CONFUI_DBG_VOLUME, -1, (val & LOG_VOLUME));
  confuiMakeItemCheckButton (gui, grid, "Web Client",
      CONFUI_DBG_WEBCLIENT, -1, (val & LOG_WEBCLIENT));
  confuiMakeItemCheckButton (gui, grid, "Web Server",
      CONFUI_DBG_WEBSRV, -1, (val & LOG_WEBSRV));
  confuiMakeItemCheckButton (gui, grid, "Backtrace",
      CONFUI_DBG_BACKTRACE, -1, (val & LOG_BACKTRACE));

  for (int i = CONFUI_DBG_IMPORTANT; i < CONFUI_DBG_MAX; ++i) {
    gui->uiitem [i].outtype = CONFUI_OUT_DEBUG;
    if (i == CONFUI_DBG_BACKTRACE) {
      gui->uiitem [i].debuglvl = LOG_BACKTRACE;
    } else {
      int         idx;
      loglevel_t  bitval;

      idx = i - CONFUI_DBG_IMPORTANT;
      bitval = 1 << idx;
      gui->uiitem [i].debuglvl = bitval;
    }
  }

  uiGridPostProcess (grid);
}

