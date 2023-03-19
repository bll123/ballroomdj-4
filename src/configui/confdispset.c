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
#include "configui.h"
#include "log.h"
#include "slist.h"
#include "ui.h"
#include "uiduallist.h"

static bool confuiDispSettingChg (void *udata);

void
confuiInitDispSettings (confuigui_t *gui)
{
  char  tbuff [MAXPATHLEN];
  char  tbuffb [MAXPATHLEN];
  char  tbuffc [MAXPATHLEN];

  /* CONTEXT: configuration: display settings for: song editor column N */
  snprintf (tbuff, sizeof (tbuff), _("Song Editor - Column %d"), 1);
  snprintf (tbuffb, sizeof (tbuffb), _("Song Editor - Column %d"), 2);
  snprintf (tbuffc, sizeof (tbuffc), _("Song Editor - Column %d"), 3);
  confuiSpinboxTextInitDataNum (gui, "cu-disp-settings",
      CONFUI_SPINBOX_DISP_SEL,
      /* CONTEXT: configuration: display settings for: music queue */
      DISP_SEL_MUSICQ, _("Music Queue"),
      /* CONTEXT: configuration: display settings for: history */
      DISP_SEL_HISTORY, _("History"),
      /* CONTEXT: configuration: display settings for: requests */
      DISP_SEL_REQUEST, _("Request"),
      /* CONTEXT: configuration: display settings for: song list */
      DISP_SEL_SONGLIST, _("Song List"),
      /* CONTEXT: configuration: display settings for: song selection */
      DISP_SEL_SONGSEL, _("Song Selection"),
      /* CONTEXT: configuration: display settings for: easy song list */
      DISP_SEL_EZSONGLIST, _("Easy Song List"),
      /* CONTEXT: configuration: display settings for: easy song selection */
      DISP_SEL_EZSONGSEL, _("Easy Song Selection"),
      /* CONTEXT: configuration: display settings for: music manager */
      DISP_SEL_MM, _("Music Manager"),
      DISP_SEL_SONGEDIT_A, tbuff,
      DISP_SEL_SONGEDIT_B, tbuffb,
      DISP_SEL_SONGEDIT_C, tbuffc,
      -1);
  gui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx = DISP_SEL_MUSICQ;
}

void
confuiBuildUIDispSettings (confuigui_t *gui)
{
  uiwcont_t    vbox;
  uiwcont_t    uiwidget;
  uiwcont_t    sg;

  logProcBegin (LOG_PROC, "confuiBuildUIDispSettings");
  uiwcontInit (&uiwidget);
  uiwcontInit (&sg);

  uiCreateVertBox (&vbox);

  /* display settings */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: change which fields are displayed in different contexts */
      _("Display Settings"), CONFUI_ID_DISP_SEL_LIST);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: display settings: which set of display settings to update */
  confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, _("Display"),
      CONFUI_SPINBOX_DISP_SEL, -1, CONFUI_OUT_NUM,
      gui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx,
      confuiDispSettingChg);

  gui->tables [CONFUI_ID_DISP_SEL_LIST].flags = CONFUI_TABLE_NONE;
  gui->tables [CONFUI_ID_DISP_SEL_TABLE].flags = CONFUI_TABLE_NONE;

  gui->dispselduallist = uiCreateDualList (&vbox,
      DUALLIST_FLAGS_NONE, NULL, NULL);
  logProcEnd (LOG_PROC, "confuiBuildUIDispSettings", "");
}

void
confuiDispSaveTable (confuigui_t *gui, int selidx)
{
  slist_t       *tlist;
  slist_t       *nlist;
  slistidx_t    val;
  slistidx_t    iteridx;
  const char    *tstr;

  logProcBegin (LOG_PROC, "confuiDispSaveTable");

  if (! uiduallistIsChanged (gui->dispselduallist)) {
    logProcEnd (LOG_PROC, "confuiDispSaveTable", "not-changed");
    return;
  }

  nlist = slistAlloc ("dispsel-save-tmp", LIST_UNORDERED, NULL);
  tlist = uiduallistGetList (gui->dispselduallist);
  slistStartIterator (tlist, &iteridx);
  while ((val = slistIterateValueNum (tlist, &iteridx)) >= 0) {
    tstr = tagdefs [val].tag;
    slistSetNum (nlist, tstr, 0);
  }

  dispselSave (gui->dispsel, selidx, nlist);

  slistFree (tlist);
  slistFree (nlist);
  logProcEnd (LOG_PROC, "confuiDispSaveTable", "");
}

/* internal routines */

static bool
confuiDispSettingChg (void *udata)
{
  confuigui_t *gui = udata;
  int         oselidx;
  int         nselidx;

  logProcBegin (LOG_PROC, "confuiDispSettingChg");


  oselidx = gui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx;
  nselidx = uiSpinboxTextGetValue (
      gui->uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);
  gui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx = nselidx;

  confuiDispSaveTable (gui, oselidx);
  /* be sure to create the listing first */
  confuiCreateTagListingDisp (gui);
  confuiCreateTagSelectedDisp (gui);
  logProcEnd (LOG_PROC, "confuiDispSettingChg", "");
  return UICB_CONT;
}



