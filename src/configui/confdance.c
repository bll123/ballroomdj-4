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
#include "bdjstring.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "dance.h"
#include "datafile.h"
#include "dnctypes.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathutil.h"
#include "slist.h"
#include "ui.h"

/* dance table */
static void confuiCreateDanceTable (confuigui_t *gui);
static int  confuiDanceEntryDanceChg (uientry_t *entry, void *udata);
static int  confuiDanceEntryTagsChg (uientry_t *entry, void *udata);
static int  confuiDanceEntryAnnouncementChg (uientry_t *entry, void *udata);
static int  confuiDanceEntryChg (uientry_t *e, void *udata, int widx);
static bool confuiDanceSpinboxTypeChg (void *udata);
static bool confuiDanceSpinboxSpeedChg (void *udata);
static bool confuiDanceSpinboxLowBPMChg (void *udata);
static bool confuiDanceSpinboxHighBPMChg (void *udata);
static bool confuiDanceSpinboxTimeSigChg (void *udata);
static void confuiDanceSpinboxChg (void *udata, int widx);
static int  confuiDanceValidateAnnouncement (uientry_t *entry, confuigui_t *gui);
static void confuiDanceSave (confuigui_t *gui);
static bool confuiSelectAnnouncement (void *udata);
static void confuiLoadDanceTypeList (confuigui_t *gui);

void
confuiInitEditDances (confuigui_t *gui)
{
  confuiLoadDanceTypeList (gui);

  confuiSpinboxTextInitDataNum (gui, "cu-dance-speed",
      CONFUI_SPINBOX_DANCE_SPEED,
      /* CONTEXT: configuration: dance speed */
      DANCE_SPEED_SLOW, _("slow"),
      /* CONTEXT: configuration: dance speed */
      DANCE_SPEED_NORMAL, _("normal"),
      /* CONTEXT: configuration: dance speed */
      DANCE_SPEED_FAST, _("fast"),
      -1);

  confuiSpinboxTextInitDataNum (gui, "cu-dance-time-sig",
      CONFUI_SPINBOX_DANCE_TIME_SIG,
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_24, _("2/4"),
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_34, _("3/4"),
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_44, _("4/4"),
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_48, _("4/8"),
      -1);
}


void
confuiBuildUIEditDances (confuigui_t *gui)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *dvbox;
  uiwcont_t     *szgrp;
  uiwcont_t     *szgrpB;
  uiwcont_t     *szgrpC;
  const char    *bpmstr;
  char          tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiBuildUIEditDances");
  gui->inchange = true;
  vbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpB = uiCreateSizeGroupHoriz ();
  szgrpC = uiCreateSizeGroupHoriz ();

  /* edit dances */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: edit the dance table */
      _("Edit Dances"), CONFUI_ID_DANCE);

  hbox = uiCreateHorizBox ();
  uiBoxPackStartExpand (vbox, hbox);
  uiWidgetAlignHorizStart (hbox);

  confuiMakeItemTable (gui, hbox, CONFUI_ID_DANCE, CONFUI_TABLE_NO_UP_DOWN);
  gui->tables [CONFUI_ID_DANCE].savefunc = confuiDanceSave;

  confuiCreateDanceTable (gui);

  dvbox = uiCreateVertBox ();
  uiWidgetSetMarginStart (dvbox, 8);
  uiBoxPackStart (hbox, dvbox);

  confuiMakeItemEntry (gui, dvbox, szgrp, tagdefs [TAG_DANCE].displayname,
      CONFUI_ENTRY_DANCE_DANCE, -1, "", CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_DANCE_DANCE].entry,
      confuiDanceEntryDanceChg, gui, UIENTRY_IMMEDIATE);
  gui->uiitem [CONFUI_ENTRY_DANCE_DANCE].danceidx = DANCE_DANCE;

  /* CONTEXT: configuration: dances: the type of the dance (club/latin/standard) */
  confuiMakeItemSpinboxText (gui, dvbox, szgrp, szgrpB, _("Type"),
      CONFUI_SPINBOX_DANCE_TYPE, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxTypeChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].danceidx = DANCE_TYPE;

  /* CONTEXT: configuration: dances: the speed of the dance (fast/normal/slow) */
  confuiMakeItemSpinboxText (gui, dvbox, szgrp, szgrpB, _("Speed"),
      CONFUI_SPINBOX_DANCE_SPEED, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxSpeedChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_SPEED].danceidx = DANCE_SPEED;

  /* CONTEXT: configuration: dances: tags associated with the dance */
  confuiMakeItemEntry (gui, dvbox, szgrp, _("Tags"),
      CONFUI_ENTRY_DANCE_TAGS, -1, "", CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_DANCE_TAGS].entry,
      confuiDanceEntryTagsChg, gui, UIENTRY_IMMEDIATE);
  gui->uiitem [CONFUI_ENTRY_DANCE_TAGS].danceidx = DANCE_TAGS;

  /* CONTEXT: configuration: dances: play the selected announcement before the dance is played */
  confuiMakeItemEntryChooser (gui, dvbox, szgrp, _("Announcement"),
      CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT, -1, "",
      confuiSelectAnnouncement);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT].entry,
      confuiDanceEntryAnnouncementChg, gui, UIENTRY_DELAYED);
  gui->uiitem [CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT].danceidx = DANCE_ANNOUNCE;

  bpmstr = tagdefs [TAG_BPM].displayname;
  /* CONTEXT: configuration: dances: low BPM (or MPM) setting */
  snprintf (tbuff, sizeof (tbuff), _("Low %s"), bpmstr);
  confuiMakeItemSpinboxNum (gui, dvbox, szgrp, szgrpC, tbuff,
      CONFUI_SPINBOX_DANCE_LOW_BPM, -1, 10, 500, 0,
      confuiDanceSpinboxLowBPMChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_LOW_BPM].danceidx = DANCE_LOW_BPM;

  /* CONTEXT: configuration: dances: high BPM (or MPM) setting */
  snprintf (tbuff, sizeof (tbuff), _("High %s"), bpmstr);
  confuiMakeItemSpinboxNum (gui, dvbox, szgrp, szgrpC, tbuff,
      CONFUI_SPINBOX_DANCE_HIGH_BPM, -1, 10, 500, 0,
      confuiDanceSpinboxHighBPMChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_HIGH_BPM].danceidx = DANCE_HIGH_BPM;

  /* CONTEXT: configuration: dances: time signature for the dance */
  confuiMakeItemSpinboxText (gui, dvbox, szgrp, szgrpC, _("Time Signature"),
      CONFUI_SPINBOX_DANCE_TIME_SIG, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxTimeSigChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_TIME_SIG].danceidx = DANCE_TIMESIG;

  gui->tables [CONFUI_ID_DANCE].callbacks [CONFUI_TABLE_CB_DANCE_SELECT] =
      callbackInitLong (confuiDanceSelect, gui);
  uiTreeViewSetRowActivatedCallback (gui->tables [CONFUI_ID_DANCE].uitree,
      gui->tables [CONFUI_ID_DANCE].callbacks [CONFUI_TABLE_CB_DANCE_SELECT]);

  uiTreeViewSelectSet (gui->tables [CONFUI_ID_DANCE].uitree, 0);
  confuiDanceSelect (gui, 0);

  gui->inchange = false;

  uiwcontFree (dvbox);
  uiwcontFree (vbox);
  uiwcontFree (hbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpB);
  uiwcontFree (szgrpC);

  logProcEnd (LOG_PROC, "confuiBuildUIEditDances", "");
}

/* internal routines */

static void
confuiCreateDanceTable (confuigui_t *gui)
{
  slistidx_t        iteridx;
  ilistidx_t        key;
  dance_t           *dances;
  uitree_t          *uitree;
  slist_t           *dancelist;


  logProcBegin (LOG_PROC, "confuiCreateDanceTable");

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  uitree = gui->tables [CONFUI_ID_DANCE].uitree;
  uiTreeViewDisableHeaders (uitree);

  uiTreeViewCreateValueStore (uitree, CONFUI_DANCE_COL_MAX,
      TREE_TYPE_STRING, TREE_TYPE_STRING, TREE_TYPE_NUM, TREE_TYPE_END);

  dancelist = danceGetDanceList (dances);
  slistStartIterator (dancelist, &iteridx);
  while ((key = slistIterateValueNum (dancelist, &iteridx)) >= 0) {
    char        *dancedisp;

    dancedisp = danceGetStr (dances, key, DANCE_DANCE);

    uiTreeViewValueAppend (uitree);
    confuiDanceSet (uitree, dancedisp, key);
    gui->tables [CONFUI_ID_DANCE].currcount += 1;
  }

  uiTreeViewAppendColumn (uitree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, "",
      TREE_COL_TYPE_TEXT, CONFUI_DANCE_COL_DANCE, TREE_COL_TYPE_END);
  uiTreeViewAppendColumn (uitree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, "",
      TREE_COL_TYPE_TEXT, CONFUI_DANCE_COL_SB_PAD, TREE_COL_TYPE_END);

  logProcEnd (LOG_PROC, "confuiCreateDanceTable", "");
}

static int
confuiDanceEntryDanceChg (uientry_t *entry, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_DANCE_DANCE);
}

static int
confuiDanceEntryTagsChg (uientry_t *entry, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_DANCE_TAGS);
}

static int
confuiDanceEntryAnnouncementChg (uientry_t *entry, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT);
}

static int
confuiDanceEntryChg (uientry_t *entry, void *udata, int widx)
{
  confuigui_t     *gui = udata;
  const char      *str;
  uitree_t        *uitree;
  int             count;
  ilistidx_t      key;
  dance_t         *dances;
  int             didx;
  datafileconv_t  conv;
  int             entryrc = UIENTRY_ERROR;

  logProcBegin (LOG_PROC, "confuiDanceEntryChg");
  if (gui->inchange) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "in-dance-select");
    return UIENTRY_OK;
  }

  str = uiEntryGetValue (entry);
  if (str == NULL) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "null-string");
    return UIENTRY_OK;
  }

  didx = gui->uiitem [widx].danceidx;

  uitree = gui->tables [CONFUI_ID_DANCE].uitree;
  count = uiTreeViewSelectGetCount (uitree);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "no-selection");
    return UIENTRY_OK;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  key = uiTreeViewGetValue (uitree, CONFUI_DANCE_COL_DANCE_IDX);

  if (widx == CONFUI_ENTRY_DANCE_DANCE) {
    uiTreeViewSetValues (uitree,
        CONFUI_DANCE_COL_DANCE, str,
        TREE_VALUE_END);
    danceSetStr (dances, key, didx, str);
    entryrc = UIENTRY_OK;
  }
  if (widx == CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT) {
    entryrc = confuiDanceValidateAnnouncement (entry, gui);
    if (entryrc == UIENTRY_OK) {
      danceSetStr (dances, key, didx, str);
    }
  }
  if (widx == CONFUI_ENTRY_DANCE_TAGS) {
    slist_t *slist;

    conv.allocated = true;
    conv.str = mdstrdup (str);
    conv.valuetype = VALUE_STR;
    convTextList (&conv);
    slist = conv.list;
    danceSetList (dances, key, didx, slist);
    entryrc = UIENTRY_OK;
  }
  if (entryrc == UIENTRY_OK) {
    gui->tables [gui->tablecurr].changed = true;
  }
  logProcEnd (LOG_PROC, "confuiDanceEntryChg", "");
  return entryrc;
}

static bool
confuiDanceSpinboxTypeChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_TYPE);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxSpeedChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_SPEED);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxLowBPMChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_LOW_BPM);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxHighBPMChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_HIGH_BPM);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxTimeSigChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_TIME_SIG);
  return UICB_CONT;
}

static void
confuiDanceSpinboxChg (void *udata, int widx)
{
  confuigui_t     *gui = udata;
  uitree_t        *uitree;
  int             count;
  double          value;
  long            nval = 0;
  ilistidx_t      key;
  dance_t         *dances;
  int             didx;

  logProcBegin (LOG_PROC, "confuiDanceSpinboxChg");
  if (gui->inchange) {
    logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "in-dance-select");
    return;
  }

  didx = gui->uiitem [widx].danceidx;

  if (gui->uiitem [widx].basetype == CONFUI_SPINBOX_TEXT) {
    /* text spinbox */
    nval = uiSpinboxTextGetValue (gui->uiitem [widx].spinbox);
  }
  if (gui->uiitem [widx].basetype == CONFUI_SPINBOX_NUM) {
    value = uiSpinboxGetValue (&gui->uiitem [widx].uiwidget);
    nval = (long) value;
  }

  uitree = gui->tables [CONFUI_ID_DANCE].uitree;
  count = uiTreeViewSelectGetCount (uitree);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "no-selection");
    return;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  key = uiTreeViewGetValue (uitree, CONFUI_DANCE_COL_DANCE_IDX);
  danceSetNum (dances, key, didx, nval);
  gui->tables [gui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "");
}

static int
confuiDanceValidateAnnouncement (uientry_t *entry, confuigui_t *gui)
{
  int               rc;
  const char        *fn;
  char              tbuff [MAXPATHLEN];
  char              nfn [MAXPATHLEN];
  char              *musicdir;
  size_t            mlen;

  logProcBegin (LOG_PROC, "confuiDanceValidateAnnouncement");

  musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
  mlen = strlen (musicdir);

  rc = UIENTRY_ERROR;
  fn = uiEntryGetValue (entry);
  if (fn == NULL) {
    logProcEnd (LOG_PROC, "confuiDanceValidateAnnouncement", "bad-fn");
    return UIENTRY_ERROR;
  }

  strlcpy (nfn, fn, sizeof (nfn));
  pathNormPath (nfn, sizeof (nfn));
  if (strncmp (musicdir, fn, mlen) == 0) {
    strlcpy (nfn, fn + mlen + 1, sizeof (nfn));
    uiEntrySetValue (entry, nfn);
  }

  if (*nfn == '\0') {
    rc = UIENTRY_OK;
  } else {
    *tbuff = '\0';
    if (*nfn != '/' && *(nfn + 1) != ':') {
      strlcpy (tbuff, musicdir, sizeof (tbuff));
      strlcat (tbuff, "/", sizeof (tbuff));
    }
    strlcat (tbuff, nfn, sizeof (tbuff));
    if (fileopFileExists (tbuff)) {
      rc = UIENTRY_OK;
    }
  }

  /* sanitizeaddress creates a buffer underflow error */
  /* if tablecurr is set to CONFUI_ID_NONE */
  /* also this validation routine gets called at most any time, but */
  /* the changed flag should only be set for the edit dance tab */
  if (gui->tablecurr == CONFUI_ID_DANCE) {
    gui->tables [gui->tablecurr].changed = true;
  }
  logProcEnd (LOG_PROC, "confuiDanceValidateAnnouncement", "");
  return rc;
}

static void
confuiDanceSave (confuigui_t *gui)
{
  dance_t   *dances;

  logProcBegin (LOG_PROC, "confuiDanceSave");

  if (gui->tables [CONFUI_ID_DANCE].changed == false) {
    logProcEnd (LOG_PROC, "confuiTableSave", "not-changed");
    return;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  /* the data is already saved in the dance list; just re-use it */
  danceSave (dances, NULL);
  logProcEnd (LOG_PROC, "confuiDanceSave", "");
}

static bool
confuiSelectAnnouncement (void *udata)
{
  confuigui_t *gui = udata;

  logProcBegin (LOG_PROC, "confuiSelectAnnouncement");
  confuiSelectFileDialog (gui, CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT,
      /* CONTEXT: configuration: announcement selection dialog: audio file filter */
      bdjoptGetStr (OPT_M_DIR_MUSIC), _("Audio Files"), "audio/*");
  logProcEnd (LOG_PROC, "confuiSelectAnnouncement", "");
  return UICB_CONT;
}

static void
confuiLoadDanceTypeList (confuigui_t *gui)
{
  nlist_t       *tlist = NULL;
  nlist_t       *llist = NULL;
  dnctype_t     *dnctypes;
  slistidx_t    iteridx;
  char          *key;
  int           count;

  logProcBegin (LOG_PROC, "confuiLoadDanceTypeList");

  tlist = nlistAlloc ("cu-dance-type", LIST_ORDERED, NULL);
  llist = nlistAlloc ("cu-dance-type-l", LIST_ORDERED, NULL);

  dnctypes = bdjvarsdfGet (BDJVDF_DANCE_TYPES);
  dnctypesStartIterator (dnctypes, &iteridx);
  count = 0;
  while ((key = dnctypesIterate (dnctypes, &iteridx)) != NULL) {
    nlistSetStr (tlist, count, key);
    nlistSetNum (llist, count, count);
    ++count;
  }

  gui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].displist = tlist;
  gui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadDanceTypeList", "");
}

