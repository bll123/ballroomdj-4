/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
/* the conversion routines use the internal status-list, but it */
/* is not necessary to update this for editing or for the save */

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
#include "bdjvarsdf.h"
#include "configui.h"
#include "log.h"
#include "mdebug.h"
#include "status.h"
#include "ui.h"

static void confuiStatusSave (confuigui_t *gui);
static void confuiStatusFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static bool confuiStatusChangeCB (void *udata);
static int confuiStatusEntryChangeCB (uiwcont_t *entry, const char *label, void *udata);
static void confuiStatusAdd (confuigui_t *gui);
static void confuiStatusRemove (confuigui_t *gui, ilistidx_t idx);
static void confuiStatusMove (confuigui_t *gui, ilistidx_t idx, int dir);

void
confuiBuildUIEditStatus (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *hbox;

  logProcBegin ();
  vbox = uiCreateVertBox ();

  /* edit status */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: edit status table */
      _("Edit Status"), CONFUI_ID_STATUS);

  hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizStart (hbox);
  uiBoxPackStartExpand (vbox, hbox);

  confuiMakeItemTable (gui, hbox, CONFUI_ID_STATUS,
      CONFUI_TABLE_KEEP_FIRST | CONFUI_TABLE_KEEP_LAST);
  gui->tables [CONFUI_ID_STATUS].savefunc = confuiStatusSave;
  gui->tables [CONFUI_ID_STATUS].addfunc = confuiStatusAdd;
  gui->tables [CONFUI_ID_STATUS].removefunc = confuiStatusRemove;
  gui->tables [CONFUI_ID_STATUS].movefunc = confuiStatusMove;
  confuiCreateStatusTable (gui);

  uiwcontFree (vbox);
  uiwcontFree (hbox);

  logProcEnd ("");
}

void
confuiCreateStatusTable (confuigui_t *gui)
{
  status_t          *status;
  uivirtlist_t      *uivl;

  logProcBegin ();

  status = bdjvarsdfGet (BDJVDF_STATUS);

  uivl = gui->tables [CONFUI_ID_STATUS].uivl;
  uivlSetNumColumns (uivl, CONFUI_STATUS_COL_MAX);
  uivlMakeColumnEntry (uivl, "status", CONFUI_STATUS_COL_STATUS, 15, 30);
  uivlMakeColumn (uivl, "pf", CONFUI_STATUS_COL_PLAY_FLAG, VL_TYPE_CHECK_BUTTON);
  uivlSetColumnHeading (uivl, CONFUI_STATUS_COL_STATUS,
      tagdefs [TAG_STATUS].displayname);
  /* CONTEXT: configuration: status: title of the "playable" column */
  uivlSetColumnHeading (uivl, CONFUI_STATUS_COL_PLAY_FLAG, _("Play?"));
  uivlSetColumnAlignCenter (uivl, CONFUI_STATUS_COL_PLAY_FLAG);
  uivlSetNumRows (uivl, statusGetCount (status));
  gui->tables [CONFUI_ID_STATUS].currcount = statusGetCount (status);

  gui->tables [CONFUI_ID_STATUS].callbacks [CONFUI_STATUS_CB] =
      callbackInit (confuiStatusChangeCB, gui, NULL);
  uivlSetEntryValidation (uivl, CONFUI_STATUS_COL_STATUS,
      confuiStatusEntryChangeCB, gui);
  uivlSetCheckboxChangeCallback (uivl, CONFUI_STATUS_COL_PLAY_FLAG,
      gui->tables [CONFUI_ID_STATUS].callbacks [CONFUI_STATUS_CB]);

  uivlSetRowFillCallback (uivl, confuiStatusFillRow, gui);
  uivlDisplay (uivl);

  logProcEnd ("");
}

/* internal routines */

static void
confuiStatusSave (confuigui_t *gui)
{
  status_t      *status;
  ilist_t       *statuslist;
  ilistidx_t    count;

  logProcBegin ();

  if (gui->tables [CONFUI_ID_STATUS].changed == false) {
    return;
  }

  status = bdjvarsdfGet (BDJVDF_STATUS);

  statuslist = ilistAlloc ("status-save", LIST_ORDERED);
  count = statusGetCount (status);
  for (int rowidx = 0; rowidx < count; ++rowidx) {
    const char  *statusdisp;
    int         playflag;

    statusdisp = statusGetStatus (status, rowidx);
    playflag = statusGetPlayFlag (status, rowidx);
    ilistSetStr (statuslist, rowidx, STATUS_STATUS, statusdisp);
    ilistSetNum (statuslist, rowidx, STATUS_PLAY_FLAG, playflag);
  }

  statusSave (status, statuslist);
  ilistFree (statuslist);
  logProcEnd ("");
}

static void
confuiStatusFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  confuigui_t   *gui = udata;
  uivirtlist_t  *uivl;
  status_t      *status;
  const char    *statusdisp;
  int           playflag;
  ilistidx_t    count;

  uivl = gui->tables [CONFUI_ID_STATUS].uivl;
  status = bdjvarsdfGet (BDJVDF_STATUS);

  count = statusGetCount (status);
  if (rownum >= count) {
    return;
  }

  gui->inchange = true;

  statusdisp = statusGetStatus (status, rownum);
  playflag = statusGetPlayFlag (status, rownum);
  uivlSetRowColumnValue (uivl, rownum, CONFUI_STATUS_COL_STATUS, statusdisp);
  uivlSetRowColumnNum (uivl, rownum, CONFUI_STATUS_COL_PLAY_FLAG, playflag);
  if (rownum == 0 || rownum == count - 1) {
    /* the first entry field, if displayed is read-only */
    /* the last entry field, if displayed, is read-only */
    uivlSetRowColumnEditable (uivl, rownum, CONFUI_STATUS_COL_STATUS, UIWIDGET_DISABLE);
  } else {
    uivlSetRowColumnEditable (uivl, rownum, CONFUI_STATUS_COL_STATUS, UIWIDGET_ENABLE);
  }

  gui->inchange = false;
}

static bool
confuiStatusChangeCB (void *udata)
{
  confuigui_t   *gui = udata;
  uivirtlist_t  *uivl;
  int32_t       rownum;
  int           playflag;
  status_t      *status;

  if (gui->inchange) {
    return UICB_CONT;
  }

  status = bdjvarsdfGet (BDJVDF_STATUS);
  uivl = gui->tables [CONFUI_ID_STATUS].uivl;
  rownum = uivlGetCurrSelection (uivl);
  playflag = uivlGetRowColumnNum (uivl, rownum, CONFUI_STATUS_COL_PLAY_FLAG);
  statusSetPlayFlag (status, rownum, playflag);
  gui->tables [CONFUI_ID_STATUS].changed = true;
  return UICB_CONT;
}

static int
confuiStatusEntryChangeCB (uiwcont_t *entry, const char *label, void *udata)
{
  confuigui_t   *gui = udata;
  uivirtlist_t  *uivl;
  int32_t       rownum;
  const char    *statusdisp;
  status_t      *status;

  if (gui->inchange) {
    return UIENTRY_OK;
  }

  status = bdjvarsdfGet (BDJVDF_STATUS);
  uivl = gui->tables [CONFUI_ID_STATUS].uivl;
  rownum = uivlGetCurrSelection (uivl);
  statusdisp = uivlGetRowColumnEntry (uivl, rownum, CONFUI_STATUS_COL_STATUS);
  statusSetStatus (status, rownum, statusdisp);
  gui->tables [CONFUI_ID_STATUS].changed = true;
  return UIENTRY_OK;
}

static void
confuiStatusAdd (confuigui_t *gui)
{
  status_t      *status;
  int           count;
  uivirtlist_t  *uivl;

  status = bdjvarsdfGet (BDJVDF_STATUS);
  uivl = gui->tables [CONFUI_ID_STATUS].uivl;

  count = statusGetCount (status);
  /* CONTEXT: configuration: status name that is set when adding a new status */
  statusSetStatus (status, count, _("New Status"));
  statusSetPlayFlag (status, count, 0);
  count += 1;
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_STATUS].currcount = count;
  uivlPopulate (uivl);

  /* the 'complete' status entry must be last */
  /* move the just added value to the prior position */
  confuiStatusMove (gui, count - 1, CONFUI_MOVE_PREV);
  /* display the last row also */
  uivlSetSelection (uivl, count - 1);
  uivlMoveSelection (uivl, VL_DIR_PREV);
}

static void
confuiStatusRemove (confuigui_t *gui, ilistidx_t delidx)
{
  status_t      *status;
  uivirtlist_t  *uivl;
  int           count;
  bool          docopy = false;

  status = bdjvarsdfGet (BDJVDF_STATUS);
  uivl = gui->tables [CONFUI_ID_STATUS].uivl;
  count = statusGetCount (status);

  for (int idx = 0; idx < count - 1; ++idx) {
    if (idx == delidx) {
      docopy = true;
    }
    if (docopy) {
      const char  *statusdisp;
      int         playflag;

      statusdisp = statusGetStatus (status, idx + 1);
      playflag = statusGetPlayFlag (status, idx + 1);
      statusSetStatus (status, idx, statusdisp);
      statusSetPlayFlag (status, idx, playflag);
    }
  }

  statusDeleteLast (status);
  count = statusGetCount (status);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_STATUS].currcount = count;
  uivlPopulate (uivl);
}

static void
confuiStatusMove (confuigui_t *gui, ilistidx_t idx, int dir)
{
  status_t      *status;
  uivirtlist_t  *uivl;
  ilistidx_t    toidx;
  char          *statusdisp;
  int           playflag;

  status = bdjvarsdfGet (BDJVDF_STATUS);
  uivl = gui->tables [CONFUI_ID_STATUS].uivl;

  toidx = idx;
  if (dir == CONFUI_MOVE_PREV) {
    toidx -= 1;
    uivlMoveSelection (uivl, VL_DIR_PREV);
  }
  if (dir == CONFUI_MOVE_NEXT) {
    toidx += 1;
    uivlMoveSelection (uivl, VL_DIR_NEXT);
  }

  statusdisp = mdstrdup (statusGetStatus (status, toidx));
  playflag = statusGetPlayFlag (status, toidx);
  statusSetStatus (status, toidx, statusGetStatus (status, idx));
  statusSetPlayFlag (status, toidx, statusGetPlayFlag (status, idx));
  statusSetStatus (status, idx, statusdisp);
  statusSetPlayFlag (status, idx, playflag);
  dataFree (statusdisp);

  uivlPopulate (uivl);

  return;
}
