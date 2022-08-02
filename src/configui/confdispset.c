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
confuiBuildUIDispSettings (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIDispSettings");
  uiutilsUIWidgetInit (&uiwidget);
  uiutilsUIWidgetInit (&sg);

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



