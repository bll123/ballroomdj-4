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

  confuiMakeItemCheckButtonG (gui, grid, "Important",
      CONFUI_DBG_IMPORTANT, -1, (val & LOG_IMPORTANT));
  confuiMakeItemCheckButtonG (gui, grid, "Basic",
      CONFUI_DBG_BASIC, -1, (val & LOG_BASIC));
  confuiMakeItemCheckButtonG (gui, grid, "Messages",
      CONFUI_DBG_MSGS, -1, (val & LOG_MSGS));
  confuiMakeItemCheckButtonG (gui, grid, "Informational",
      CONFUI_DBG_INFO, -1, (val & LOG_INFO));
  confuiMakeItemCheckButtonG (gui, grid, "Actions",
      CONFUI_DBG_ACTIONS, -1, (val & LOG_ACTIONS));
  confuiMakeItemCheckButtonG (gui, grid, "Audio Adjust",
      CONFUI_DBG_AUDIO_ADJUST, -1, (val & LOG_AUDIO_ADJUST));
  confuiMakeItemCheckButtonG (gui, grid, "Audio ID",
      CONFUI_DBG_AUDIO_ID, -1, (val & LOG_AUDIO_ID));
  confuiMakeItemCheckButtonG (gui, grid, "Audio ID Dump",
      CONFUI_DBG_AUDIOID_DUMP, -1, (val & LOG_AUDIOID_DUMP));
  confuiMakeItemCheckButtonG (gui, grid, "Audio Source",
      CONFUI_DBG_AUDIOSRC, -1, (val & LOG_AUDIOSRC));
  confuiMakeItemCheckButtonG (gui, grid, "Audio Tags",
      CONFUI_DBG_AUDIO_TAG, -1, (val & LOG_AUDIO_TAG));

  uiGridNewCol (grid);
  confuiMakeItemCheckButtonG (gui, grid, "Dance Selection",
      CONFUI_DBG_DANCESEL, -1, (val & LOG_DANCESEL));
  confuiMakeItemCheckButtonG (gui, grid, "Database",
      CONFUI_DBG_DB, -1, (val & LOG_DB));
  confuiMakeItemCheckButtonG (gui, grid, "Database Update",
      CONFUI_DBG_DBUPDATE, -1, (val & LOG_DBUPDATE));
  confuiMakeItemCheckButtonG (gui, grid, "Datafile",
      CONFUI_DBG_DATAFILE, -1, (val & LOG_DATAFILE));
  confuiMakeItemCheckButtonG (gui, grid, "Grouping",
      CONFUI_DBG_GROUPING, -1, (val & LOG_GROUPING));
  confuiMakeItemCheckButtonG (gui, grid, "iTunes",
      CONFUI_DBG_ITUNES, -1, (val & LOG_ITUNES));
  confuiMakeItemCheckButtonG (gui, grid, "List",
      CONFUI_DBG_LIST, -1, (val & LOG_LIST));
  confuiMakeItemCheckButtonG (gui, grid, "Player",
      CONFUI_DBG_PLAYER, -1, (val & LOG_PLAYER));
  confuiMakeItemCheckButtonG (gui, grid, "Procedures",
      CONFUI_DBG_PROC, -1, (val & LOG_PROC));
  confuiMakeItemCheckButtonG (gui, grid, "Process",
      CONFUI_DBG_PROCESS, -1, (val & LOG_PROCESS));

  uiGridNewCol (grid);
  confuiMakeItemCheckButtonG (gui, grid, "Program State",
      CONFUI_DBG_PROGSTATE, -1, (val & LOG_PROGSTATE));
  confuiMakeItemCheckButtonG (gui, grid, "Random Access File",
      CONFUI_DBG_RAFILE, -1, (val & LOG_RAFILE));
  confuiMakeItemCheckButtonG (gui, grid, "Socket",
      CONFUI_DBG_SOCKET, -1, (val & LOG_SOCKET));
  confuiMakeItemCheckButtonG (gui, grid, "Song Selection",
      CONFUI_DBG_SONGSEL, -1, (val & LOG_SONGSEL));
  confuiMakeItemCheckButtonG (gui, grid, "User Interface",
      CONFUI_DBG_UI, -1, (val & LOG_UI));
  confuiMakeItemCheckButtonG (gui, grid, "Virtual List",
      CONFUI_DBG_VIRTLIST, -1, (val & LOG_VIRTLIST));
  confuiMakeItemCheckButtonG (gui, grid, "Volume",
      CONFUI_DBG_VOLUME, -1, (val & LOG_VOLUME));
  confuiMakeItemCheckButtonG (gui, grid, "Web Client",
      CONFUI_DBG_WEBCLIENT, -1, (val & LOG_WEBCLIENT));
  confuiMakeItemCheckButtonG (gui, grid, "Web Server",
      CONFUI_DBG_WEBSRV, -1, (val & LOG_WEBSRV));
  confuiMakeItemCheckButtonG (gui, grid, "Backtrace",
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

