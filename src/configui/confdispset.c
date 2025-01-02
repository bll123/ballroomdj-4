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
#include "configui.h"
#include "log.h"
#include "slist.h"
#include "ui.h"
#include "uiduallist.h"

static bool confuiDispSettingChg (void *udata);

void
confuiInitDispSettings (confuigui_t *gui)
{
  char  tbuffse [DISP_SEL_SONGEDIT_MAX][MAXPATHLEN];

  for (int i = 0; i < DISP_SEL_SONGEDIT_MAX; ++i) {
    /* CONTEXT: configuration: display settings for: song editor column N */
    snprintf (tbuffse [i], MAXPATHLEN, _("Song Editor - Column %d"), i + 1);
  }
  /* as this list is set up manually, it will ignore */
  /* the disp-sel-max-player marker */
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
      /* CONTEXT: configuration: display settings for: side-by-side song list (suggestion: combined view: song list) */
      DISP_SEL_SBS_SONGLIST, _("Side-by-Side Song List"),
      /* CONTEXT: configuration: display settings for: side-by-side song selection (suggestion: combined view: song selection) */
      DISP_SEL_SBS_SONGSEL, _("Side-by-Side Song Selection"),
      /* CONTEXT: configuration: display settings for: music manager */
      DISP_SEL_MM, _("Music Manager"),
      DISP_SEL_SONGEDIT_A, tbuffse [DISP_SEL_SONGEDIT_A - DISP_SEL_SONGEDIT_A],
      DISP_SEL_SONGEDIT_B, tbuffse [DISP_SEL_SONGEDIT_B - DISP_SEL_SONGEDIT_A],
      DISP_SEL_SONGEDIT_C, tbuffse [DISP_SEL_SONGEDIT_C - DISP_SEL_SONGEDIT_A],
      /* CONTEXT: configuration: display settings for: Audio ID match listing */
      DISP_SEL_AUDIOID_LIST, _("Audio ID Match List"),
      /* CONTEXT: configuration: display settings for: Audio Identification */
      DISP_SEL_AUDIOID, _("Audio ID"),
      /* CONTEXT: configuration: display settings for: Marquee (song display screen) */
      DISP_SEL_MARQUEE, _("Marquee"),
      /* CONTEXT: configuration: display settings for: Player User Interface */
      DISP_SEL_CURRSONG, _("Current Song"),
      -1);
  gui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx = DISP_SEL_MUSICQ;
}

void
confuiBuildUIDispSettings (confuigui_t *gui)
{
  uiwcont_t    *vbox;

  logProcBegin ();

  vbox = uiCreateVertBox ();

  /* display settings */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: change which fields are displayed in different contexts */
      _("Display Settings"), CONFUI_ID_DISP_SEL_LIST);

  /* CONTEXT: configuration: display settings: which set of display settings to update */
  confuiMakeItemSpinboxText (gui, vbox, NULL, NULL, _("Display"),
      CONFUI_SPINBOX_DISP_SEL, -1, CONFUI_OUT_NUM,
      gui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx,
      confuiDispSettingChg);

  gui->tables [CONFUI_ID_DISP_SEL_LIST].flags = CONFUI_TABLE_NONE;
  gui->tables [CONFUI_ID_DISP_SEL_TABLE].flags = CONFUI_TABLE_NONE;

  gui->dispselduallist = uiCreateDualList (vbox,
      DL_FLAGS_NONE, NULL, NULL);

  uiwcontFree (vbox);
  logProcEnd ("");
}

void
confuiDispSaveTable (confuigui_t *gui, int selidx)
{
  slist_t       *tlist;
  slist_t       *nlist;
  slistidx_t    val;
  slistidx_t    iteridx;
  const char    *tstr;

  logProcBegin ();

  if (! uiduallistIsChanged (gui->dispselduallist)) {
    logProcEnd ("not-changed");
    return;
  }

  nlist = slistAlloc ("dispsel-save-tmp", LIST_UNORDERED, NULL);
  tlist = uiduallistGetList (gui->dispselduallist);
  slistStartIterator (tlist, &iteridx);
  while ((val = slistIterateValueNum (tlist, &iteridx)) >= 0) {
    if (selidx == DISP_SEL_AUDIOID && val == TAG_AUDIOID_SCORE) {
      continue;
    }
    tstr = tagdefs [val].tag;
    slistSetNum (nlist, tstr, 0);
  }

  dispselSave (gui->dispsel, selidx, nlist);

  slistFree (tlist);
  slistFree (nlist);
  logProcEnd ("");
}

/* internal routines */

static bool
confuiDispSettingChg (void *udata)
{
  confuigui_t *gui = udata;
  int         oselidx;
  int         nselidx;

  logProcBegin ();


  oselidx = gui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx;
  nselidx = uiSpinboxTextGetValue (
      gui->uiitem [CONFUI_SPINBOX_DISP_SEL].uiwidgetp);
  gui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx = nselidx;

  confuiDispSaveTable (gui, oselidx);
  /* be sure to create the selected list (target) first */
  confuiCreateTagSelectedDisp (gui);
  confuiCreateTagListingDisp (gui);
  logProcEnd ("");
  return UICB_CONT;
}
