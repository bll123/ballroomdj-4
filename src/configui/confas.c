/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include "asconf.h"
#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "datafile.h"
#include "dnctypes.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "slist.h"
#include "ui.h"
#include "uiselectfile.h"
#include "uivirtlist.h"

static void confuiCreateAudioSrcTable (confuigui_t *gui);
static int  confuiAudioSrcNameChg (uiwcont_t *entry, const char *label, void *udata);
static int  confuiAudioSrcURIChg (uiwcont_t *entry, const char *label, void *udata);
static int  confuiAudioSrcUserChg (uiwcont_t *entry, const char *label, void *udata);
static int  confuiAudioSrcPassChg (uiwcont_t *entry, const char *label, void *udata);
static int  confuiAudioSrcEntryChg (uiwcont_t *e, void *udata, int widx);
static bool confuiAudioSrcModeChg (void *udata);
static bool confuiAudioSrcTypeChg (void *udata);
static bool confuiAudioSrcPortChg (void *udata);
static bool confuiAudioSrcChkConn (void *udata);
static void confuiAudioSrcSpinboxChg (void *udata, int widx);
static void confuiAudioSrcSave (confuigui_t *gui);
static void confuiAudioSrcFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static void confuiAudioSrcSelect (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static void confuiAudioSrcRemove (confuigui_t *gui, ilistidx_t idx);
static void confuiAudioSrcAdd (confuigui_t *gui);
static void confuiAudioSrcSetWidgetStates (confuigui_t *gui, int askey);

void
confuiInitAudioSource (confuigui_t *gui)
{
  confuiSpinboxTextInitDataNum (gui, "cu-as-type",
      CONFUI_SPINBOX_AUDIOSRC_MODE,
      /* CONTEXT: configuration: audio source mode */
      ASCONF_MODE_OFF, _("Off"),
      /* CONTEXT: configuration: audio source mode */
      ASCONF_MODE_CLIENT, _("Client"),
      /* CONTEXT: configuration: audio source mode */
      ASCONF_MODE_SERVER, _("Server"),
      -1);

  confuiSpinboxTextInitDataNum (gui, "cu-as-type",
      CONFUI_SPINBOX_AUDIOSRC_TYPE,
      /* CONTEXT: configuration: audio source type */
      AUDIOSRC_TYPE_BDJ4, _("bdj4"),
      /* CONTEXT: configuration: audio source type */
      AUDIOSRC_TYPE_RTSP, _("rtsp"),
      -1);

  gui->tables [CONFUI_ID_AUDIOSRC].addfunc = confuiAudioSrcAdd;
  gui->tables [CONFUI_ID_AUDIOSRC].removefunc = confuiAudioSrcRemove;
  gui->asconf = asconfAlloc ();
}

void
confuiCleanAudioSource (confuigui_t *gui)
{
  if (gui->asconf != NULL) {
    asconfFree (gui->asconf);
  }
}

void
confuiBuildUIAudioSource (confuigui_t *gui)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *dvbox;
  uiwcont_t     *szgrp;
  uiwcont_t     *szgrpB;
  uiwcont_t     *szgrpC;
  uivirtlist_t  *uivl;

  logProcBegin ();
  gui->inchange = true;
  vbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpB = uiCreateSizeGroupHoriz ();
  szgrpC = uiCreateSizeGroupHoriz ();

  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: audio sources */
      _("Audio Sources"), CONFUI_ID_AUDIOSRC);

  hbox = uiCreateHorizBox ();
  uiBoxPackStartExpand (vbox, hbox);
  uiWidgetAlignHorizStart (hbox);

  confuiMakeItemTable (gui, hbox, CONFUI_ID_AUDIOSRC, CONFUI_TABLE_NO_UP_DOWN);
  gui->tables [CONFUI_ID_AUDIOSRC].savefunc = confuiAudioSrcSave;

  confuiCreateAudioSrcTable (gui);

  dvbox = uiCreateVertBox ();
  uiBoxPackStart (hbox, dvbox);
  uiWidgetSetMarginStart (dvbox, 8);

  /* CONTEXT: configuration: audio source: mode (off/client/server) */
  confuiMakeItemSpinboxText (gui, dvbox, szgrp, szgrpB, _("Mode"),
      CONFUI_SPINBOX_AUDIOSRC_MODE, -1, CONFUI_OUT_NUM, CONFUI_NO_INDENT,
      confuiAudioSrcModeChg);
  gui->uiitem [CONFUI_SPINBOX_AUDIOSRC_MODE].audiosrcitemidx = ASCONF_MODE;

  /* CONTEXT: configuration: the remote BDJ4 server name */
  confuiMakeItemEntry (gui, dvbox, szgrp, _("Name"),
      CONFUI_ENTRY_AUDIOSRC_NAME, -1, "", CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_NAME].uiwidgetp,
      "", confuiAudioSrcNameChg, gui, UIENTRY_IMMEDIATE);
  gui->uiitem [CONFUI_ENTRY_AUDIOSRC_NAME].audiosrcitemidx = ASCONF_NAME;

  /* CONTEXT: configuration: audio source: type of source */
  confuiMakeItemSpinboxText (gui, dvbox, szgrp, szgrpB, _("Type"),
      CONFUI_SPINBOX_AUDIOSRC_TYPE, -1, CONFUI_OUT_NUM, CONFUI_NO_INDENT,
      confuiAudioSrcTypeChg);
  gui->uiitem [CONFUI_SPINBOX_AUDIOSRC_TYPE].audiosrcitemidx = ASCONF_TYPE;

  /* CONTEXT: configuration: the remote BDJ4 server name */
  confuiMakeItemEntry (gui, dvbox, szgrp, _("URI"),
      CONFUI_ENTRY_AUDIOSRC_URI, -1, "", CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_URI].uiwidgetp,
      "", confuiAudioSrcURIChg, gui, UIENTRY_IMMEDIATE);
  gui->uiitem [CONFUI_ENTRY_AUDIOSRC_URI].audiosrcitemidx = ASCONF_URI;

  /* CONTEXT: configuration: the port to use for the BDJ4 server */
  confuiMakeItemSpinboxNum (gui, dvbox, szgrp, NULL, _("Port"),
      CONFUI_WIDGET_AUDIOSRC_PORT, -1,
      8000, 30000, 0, confuiAudioSrcPortChg);
  gui->uiitem [CONFUI_WIDGET_AUDIOSRC_PORT].audiosrcitemidx = ASCONF_PORT;

  /* CONTEXT: configuration: the BDJ4 server user */
  confuiMakeItemEntry (gui, dvbox, szgrp, _("User"),
      CONFUI_ENTRY_AUDIOSRC_USER, -1, "", CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_USER].uiwidgetp,
      "", confuiAudioSrcUserChg, gui, UIENTRY_IMMEDIATE);
  gui->uiitem [CONFUI_ENTRY_AUDIOSRC_USER].audiosrcitemidx = ASCONF_USER;

  /* CONTEXT: configuration: the BDJ4 server password */
  confuiMakeItemEntry (gui, dvbox, szgrp, _("Password"),
      CONFUI_ENTRY_AUDIOSRC_PASS, -1, "", CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_PASS].uiwidgetp,
      "", confuiAudioSrcPassChg, gui, UIENTRY_IMMEDIATE);
  gui->uiitem [CONFUI_ENTRY_AUDIOSRC_PASS].audiosrcitemidx = ASCONF_PASS;

  /* CONTEXT: configuration: check connection for audio source */
  confuiMakeItemButton (gui, dvbox, szgrp, _("Check Connection"),
      CONFUI_WIDGET_AUDIOSRC_CHK_CONN, -1, confuiAudioSrcChkConn);

  uivlSetSelectChgCallback (gui->tables [CONFUI_ID_AUDIOSRC].uivl,
      confuiAudioSrcSelect, gui);

  gui->inchange = false;

  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  confuiAudioSrcSelect (gui, uivl, 0, CONFUI_AUDIOSRC_COL_NAME);

  uiwcontFree (dvbox);
  uiwcontFree (vbox);
  uiwcontFree (hbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpB);
  uiwcontFree (szgrpC);

  logProcEnd ("");
}

void
confuiAudioSrcSelectLoadValues (confuigui_t *gui, ilistidx_t askey)
{
  const char      *sval;
  int             widx;
  nlistidx_t      num;
  uivirtlist_t    *uivl;
  int32_t         rownum;

  if (askey < 0) {
    return;
  }

  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  rownum = uivlGetCurrSelection (uivl);

  /* the key must be saved because it gets changed before the */
  /* 'changed' callbacks get called */
  /* this occurs when typing into a field, then selecting a new name */
  gui->asconfkey = uivlGetRowColumnNum (uivl, rownum, CONFUI_AUDIOSRC_COL_KEY);

  sval = asconfGetStr (gui->asconf, askey, ASCONF_NAME);
  widx = CONFUI_ENTRY_AUDIOSRC_NAME;
  uiEntrySetValue (gui->uiitem [widx].uiwidgetp, sval);
  /* because the same entry field is used when switching items */
  /* and there is a validation timer running, */
  /* the validation timer must be cleared */
  /* the entry field does not need to be validated when being loaded */
  uiEntryValidateClear (gui->uiitem [widx].uiwidgetp);

  num = asconfGetNum (gui->asconf, askey, ASCONF_MODE);
  widx = CONFUI_SPINBOX_AUDIOSRC_MODE;
  uiSpinboxTextSetValue (gui->uiitem [widx].uiwidgetp, num);

  num = asconfGetNum (gui->asconf, askey, ASCONF_TYPE);
  widx = CONFUI_SPINBOX_AUDIOSRC_TYPE;
  uiSpinboxTextSetValue (gui->uiitem [widx].uiwidgetp, num);

  num = asconfGetNum (gui->asconf, askey, ASCONF_PORT);
  widx = CONFUI_WIDGET_AUDIOSRC_PORT;
  uiSpinboxSetValue (gui->uiitem [widx].uiwidgetp, num);

  sval = asconfGetStr (gui->asconf, askey, ASCONF_URI);
  widx = CONFUI_ENTRY_AUDIOSRC_URI;
  uiEntrySetValue (gui->uiitem [widx].uiwidgetp, sval);
  uiEntryValidateClear (gui->uiitem [widx].uiwidgetp);

  sval = asconfGetStr (gui->asconf, askey, ASCONF_USER);
  widx = CONFUI_ENTRY_AUDIOSRC_USER;
  uiEntrySetValue (gui->uiitem [widx].uiwidgetp, sval);
  uiEntryValidateClear (gui->uiitem [widx].uiwidgetp);

  sval = asconfGetStr (gui->asconf, askey, ASCONF_PASS);
  widx = CONFUI_ENTRY_AUDIOSRC_PASS;
  uiEntrySetValue (gui->uiitem [widx].uiwidgetp, sval);
  uiEntryValidateClear (gui->uiitem [widx].uiwidgetp);
}

void
confuiAudioSrcSearchSelect (confuigui_t *gui, ilistidx_t askey)
{
  slist_t       *asconflist;
  const char    *name;
  int32_t       idx;
  uivirtlist_t  *uivl;

  asconflist = asconfGetAudioSourceList (gui->asconf);
  name = asconfGetStr (gui->asconf, askey, ASCONF_NAME);
  idx = slistGetIdx (asconflist, name);
  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  uivlSetSelection (uivl, idx);
}

/* internal routines */

static void
confuiCreateAudioSrcTable (confuigui_t *gui)
{
  uivirtlist_t      *uivl;
  ilistidx_t        count;


  logProcBegin ();

  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  uivlSetDarkBackground (uivl);
  uivlSetNumColumns (uivl, CONFUI_AUDIOSRC_COL_MAX);
  uivlMakeColumn (uivl, "name", CONFUI_AUDIOSRC_COL_NAME, VL_TYPE_LABEL);
  uivlSetColumnGrow (uivl, CONFUI_AUDIOSRC_COL_NAME, VL_COL_WIDTH_GROW_ONLY);
  uivlMakeColumn (uivl, "askey", CONFUI_AUDIOSRC_COL_KEY, VL_TYPE_INTERNAL_NUMERIC);
  count = asconfGetCount (gui->asconf);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_AUDIOSRC].currcount = count;
  uivlSetRowFillCallback (uivl, confuiAudioSrcFillRow, gui);
  uivlDisplay (uivl);

  logProcEnd ("");
}

static int
confuiAudioSrcNameChg (uiwcont_t *entry, const char *label, void *udata)
{
  return confuiAudioSrcEntryChg (entry, udata, CONFUI_ENTRY_AUDIOSRC_NAME);
}

static int
confuiAudioSrcURIChg (uiwcont_t *entry, const char *label, void *udata)
{
  return confuiAudioSrcEntryChg (entry, udata, CONFUI_ENTRY_AUDIOSRC_URI);
}

static int
confuiAudioSrcUserChg (uiwcont_t *entry, const char *label, void *udata)
{
  return confuiAudioSrcEntryChg (entry, udata, CONFUI_ENTRY_AUDIOSRC_USER);
}

static int
confuiAudioSrcPassChg (uiwcont_t *entry, const char *label, void *udata)
{
  return confuiAudioSrcEntryChg (entry, udata, CONFUI_ENTRY_AUDIOSRC_PASS);
}

static int
confuiAudioSrcEntryChg (uiwcont_t *entry, void *udata, int widx)
{
  confuigui_t     *gui = udata;
  const char      *str = NULL;
  uivirtlist_t    *uivl = NULL;
  int             count = 0;
  ilistidx_t      askey;
  nlistidx_t      itemidx;
  int             entryrc = UIENTRY_ERROR;

  logProcBegin ();
  if (gui->inchange) {
    logProcEnd ("in-asconf-select");
    return UIENTRY_OK;
  }
  if (gui->tablecurr != CONFUI_ID_AUDIOSRC) {
    logProcEnd ("not-table-asconf");
    return UIENTRY_OK;
  }

  /* note that in gtk this is a constant pointer to the current value */
  /* and the value changes due to the validation */
  str = uiEntryGetValue (entry);
  if (str == NULL) {
    logProcEnd ("null-string");
    return UIENTRY_OK;
  }

  itemidx = gui->uiitem [widx].audiosrcitemidx;

  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  count = uivlSelectionCount (uivl);
  if (count != 1) {
    logProcEnd ("no-selection");
    return UIENTRY_OK;
  }

  askey = gui->asconfkey;

  asconfSetStr (gui->asconf, askey, itemidx, str);
  if (widx == CONFUI_ENTRY_AUDIOSRC_NAME) {
    uivlPopulate (uivl);
  }
  entryrc = UIENTRY_OK;
  if (entryrc == UIENTRY_OK) {
    gui->tables [gui->tablecurr].changed = true;
  }

  logProcEnd ("");
  return entryrc;
}

static bool
confuiAudioSrcTypeChg (void *udata)
{
  confuiAudioSrcSpinboxChg (udata, CONFUI_SPINBOX_AUDIOSRC_TYPE);
  return UICB_CONT;
}

static bool
confuiAudioSrcModeChg (void *udata)
{
  confuiAudioSrcSpinboxChg (udata, CONFUI_SPINBOX_AUDIOSRC_MODE);
  return UICB_CONT;
}

static bool
confuiAudioSrcPortChg (void *udata)
{
  confuiAudioSrcSpinboxChg (udata, CONFUI_WIDGET_AUDIOSRC_PORT);
  return UICB_CONT;
}

static void
confuiAudioSrcSpinboxChg (void *udata, int widx)
{
  confuigui_t     *gui = udata;
  uivirtlist_t    *uivl = NULL;
  int             count = 0;
  int32_t         nval = 0;
  ilistidx_t      askey;
  nlistidx_t      itemidx;

  logProcBegin ();
  if (gui->inchange) {
    logProcEnd ("in-asconf-select");
    return;
  }
  if (gui->tablecurr != CONFUI_ID_AUDIOSRC) {
    logProcEnd ("not-table-asconf");
    return;
  }

  itemidx = gui->uiitem [widx].audiosrcitemidx;

  if (gui->uiitem [widx].basetype == CONFUI_SPINBOX_TEXT) {
    /* text spinbox */
    nval = uiSpinboxTextGetValue (gui->uiitem [widx].uiwidgetp);
  }
  if (gui->uiitem [widx].basetype == CONFUI_SPINBOX_NUM) {
    double    value;

    value = uiSpinboxGetValue (gui->uiitem [widx].uiwidgetp);
    nval = (int32_t) value;
  }

  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  count = uivlSelectionCount (uivl);
  if (count != 1) {
    logProcEnd ("no-selection");
    return;
  }

  askey = gui->asconfkey;
  asconfSetNum (gui->asconf, askey, itemidx, nval);
  gui->tables [gui->tablecurr].changed = true;
  if (widx == CONFUI_SPINBOX_AUDIOSRC_MODE) {
    int   mode;

    confuiAudioSrcSetWidgetStates (gui, askey);
    mode = asconfGetNum (gui->asconf, askey, ASCONF_MODE);
    if (mode == ASCONF_MODE_SERVER) {
      int   widx;

      widx = CONFUI_SPINBOX_AUDIOSRC_TYPE;
      uiSpinboxTextSetValue (gui->uiitem [widx].uiwidgetp, AUDIOSRC_TYPE_BDJ4);
    }
  }
  logProcEnd ("");
}

static void
confuiAudioSrcSave (confuigui_t *gui)
{
  logProcBegin ();

  if (gui->tables [CONFUI_ID_AUDIOSRC].changed == false) {
    logProcEnd ("not-changed");
    return;
  }

  /* the data is already saved in the list; just re-use it */
  asconfSave (gui->asconf, NULL, -1);
  logProcEnd ("");
}

static void
confuiAudioSrcFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  confuigui_t *gui = udata;
  slist_t     *asconflist;
  const char  *name;
  slistidx_t  askey;

  asconflist = asconfGetAudioSourceList (gui->asconf);
  askey = slistGetNumByIdx (asconflist, rownum);
  if (askey == LIST_VALUE_INVALID) {
    return;
  }

  name = asconfGetStr (gui->asconf, askey, ASCONF_NAME);
  uivlSetRowColumnStr (gui->tables [CONFUI_ID_AUDIOSRC].uivl, rownum,
      CONFUI_AUDIOSRC_COL_NAME, name);
  uivlSetRowColumnNum (gui->tables [CONFUI_ID_AUDIOSRC].uivl, rownum,
      CONFUI_AUDIOSRC_COL_KEY, askey);
}

static void
confuiAudioSrcSelect (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx)
{
  confuigui_t   *gui = udata;
  uivirtlist_t  *uivl;
  ilistidx_t    askey;

  if (gui->inchange) {
    logProcEnd ("in-asconf-select");
    return;
  }

  gui->inchange = true;
  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  if (uivl == NULL) {
    return;
  }

  askey = uivlGetRowColumnNum (uivl, rownum, CONFUI_AUDIOSRC_COL_KEY);
  confuiAudioSrcSelectLoadValues (gui, askey);
  confuiAudioSrcSetWidgetStates (gui, askey);
  gui->inchange = false;
}

static void
confuiAudioSrcRemove (confuigui_t *gui, ilistidx_t rowidx)
{
  uivirtlist_t  *uivl;
  ilistidx_t    askey;
  ilistidx_t    count;

  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;

  askey = uivlGetRowColumnNum (uivl, rowidx, CONFUI_AUDIOSRC_COL_KEY);
  asconfDelete (gui->asconf, askey);
  asconfSave (gui->asconf, NULL, -1);
  count = asconfGetCount (gui->asconf);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_AUDIOSRC].currcount = count;
  uivlPopulate (uivl);
  if (rowidx >= count) {
    --rowidx;
  }
  askey = uivlGetRowColumnNum (uivl, rowidx, CONFUI_AUDIOSRC_COL_KEY);
  confuiAudioSrcSelectLoadValues (gui, askey);
}

static void
confuiAudioSrcAdd (confuigui_t *gui)
{
  ilistidx_t    askey;
  uivirtlist_t  *uivl;
  ilistidx_t    count;

  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;

  /* CONTEXT: configuration: name that is set when adding a new audio source */
  askey = asconfAdd (gui->asconf, _("New Audio Source"));
  asconfSave (gui->asconf, NULL, -1);
  count = asconfGetCount (gui->asconf);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_AUDIOSRC].currcount = count;
  uivlPopulate (uivl);
  confuiAudioSrcSearchSelect (gui, askey);
}

static void
confuiAudioSrcSetWidgetStates (confuigui_t *gui, int askey)
{
  int   mode;
  int   state;
  int   ustate;

  mode = asconfGetNum (gui->asconf, askey, ASCONF_MODE);
  state = UIWIDGET_DISABLE;
  ustate = UIWIDGET_DISABLE;
  if (mode != ASCONF_MODE_OFF) {
    state = UIWIDGET_ENABLE;
  }
  if (mode == ASCONF_MODE_CLIENT) {
    ustate = UIWIDGET_ENABLE;
  }

  uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_NAME].uilabelp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_SPINBOX_AUDIOSRC_TYPE].uilabelp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_URI].uilabelp, ustate);
  uiWidgetSetState (gui->uiitem [CONFUI_WIDGET_AUDIOSRC_PORT].uilabelp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_USER].uilabelp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_PASS].uilabelp, state);

  uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_NAME].uiwidgetp, state);
  uiSpinboxSetState (gui->uiitem [CONFUI_SPINBOX_AUDIOSRC_TYPE].uiwidgetp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_URI].uiwidgetp, ustate);
  uiWidgetSetState (gui->uiitem [CONFUI_WIDGET_AUDIOSRC_PORT].uiwidgetp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_USER].uiwidgetp, state);
  uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_AUDIOSRC_PASS].uiwidgetp, state);
}

static bool
confuiAudioSrcChkConn (void *udata)
{
  confuigui_t   *gui = udata;
  bool          rc = false;


  confuiAudioSrcSave (gui);
  rc = audiosrcCheckConnection (gui->asconfkey);
  uiLabelSetText (gui->statusMsg, "");
  if (rc == false) {
    /* CONTEXT: configuration: audio source: check connection status */
    uiLabelSetText (gui->statusMsg, _("Connection Failed"));
  } else {
    /* CONTEXT: configuration: audio source: check connection status */
    uiLabelSetText (gui->statusMsg, _("Connection OK"));
  }

  return UICB_CONT;
}
