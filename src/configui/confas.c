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

/* dance table */
static void confuiCreateDanceTable (confuigui_t *gui);
static int  confuiDanceEntryDanceChg (uiwcont_t *entry, const char *label, void *udata);
static int  confuiDanceEntryChg (uiwcont_t *e, void *udata, int widx);
static bool confuiDanceSpinboxTypeChg (void *udata);
static void confuiDanceSpinboxChg (void *udata, int widx);
static void confuiDanceSave (confuigui_t *gui);
static void confuiLoadDanceTypeList (confuigui_t *gui);
static void confuiDanceFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static void confuiDanceSelect (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static void confuiDanceRemove (confuigui_t *gui, ilistidx_t idx);
static void confuiDanceAdd (confuigui_t *gui);

void
confuiInitAudioSource (confuigui_t *gui)
{
  gui->tables [CONFUI_ID_AUDIOSRC].addfunc = confuiDanceAdd;
  gui->tables [CONFUI_ID_AUDIOSRC].removefunc = confuiDanceRemove;
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
  const char    *bpmstr;
  char          tbuff [MAXPATHLEN];
  uivirtlist_t  *uivl;

  logProcBegin ();
  gui->inchange = true;
  vbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpB = uiCreateSizeGroupHoriz ();
  szgrpC = uiCreateSizeGroupHoriz ();

  /* edit dances */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: audio sources */
      _("Audio Source"), CONFUI_ID_AUDIOSRC);

  hbox = uiCreateHorizBox ();
  uiBoxPackStartExpand (vbox, hbox);
  uiWidgetAlignHorizStart (hbox);

  confuiMakeItemTable (gui, hbox, CONFUI_ID_AUDIOSRC, CONFUI_TABLE_NO_UP_DOWN);
  gui->tables [CONFUI_ID_AUDIOSRC].savefunc = confuiAudioSourceSave;

//  confuiCreateDanceTable (gui);

  dvbox = uiCreateVertBox ();
  uiBoxPackStart (hbox, dvbox);
  uiWidgetSetMarginStart (dvbox, 8);

#if 0
  confuiMakeItemEntry (gui, dvbox, szgrp, tagdefs [TAG_DANCE].displayname,
      CONFUI_ENTRY_DANCE_DANCE, -1, "", CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_DANCE_DANCE].uiwidgetp,
      "", confuiDanceEntryDanceChg, gui, UIENTRY_IMMEDIATE);
  gui->uiitem [CONFUI_ENTRY_DANCE_DANCE].danceitemidx = DANCE_DANCE;

  /* CONTEXT: configuration: dances: the type of the dance (club/latin/standard) */
  confuiMakeItemSpinboxText (gui, dvbox, szgrp, szgrpB, _("Type"),
      CONFUI_SPINBOX_DANCE_TYPE, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxTypeChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].danceitemidx = DANCE_TYPE;

  /* CONTEXT: configuration: dances: the speed of the dance (fast/normal/slow) */
  confuiMakeItemSpinboxText (gui, dvbox, szgrp, szgrpB, _("Speed"),
      CONFUI_SPINBOX_DANCE_SPEED, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxSpeedChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_SPEED].danceitemidx = DANCE_SPEED;

  /* CONTEXT: configuration: dances: tags associated with the dance */
  confuiMakeItemEntry (gui, dvbox, szgrp, _("Tags"),
      CONFUI_ENTRY_DANCE_TAGS, -1, "", CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_DANCE_TAGS].uiwidgetp,
      "", confuiDanceEntryTagsChg, gui, UIENTRY_IMMEDIATE);
  gui->uiitem [CONFUI_ENTRY_DANCE_TAGS].danceitemidx = DANCE_TAGS;

  /* CONTEXT: configuration: dances: play the selected announcement before the dance is played */
  confuiMakeItemEntryChooser (gui, dvbox, szgrp, _("Announcement"),
      CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT, -1, "",
      selectAudioFileCallback);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT].uiwidgetp,
      "", confuiDanceEntryAnnouncementChg, gui, UIENTRY_DELAYED);
  gui->uiitem [CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT].danceitemidx = DANCE_ANNOUNCE;

  bpmstr = tagdefs [TAG_BPM].displayname;
  /* CONTEXT: configuration: dances: low BPM (or MPM) setting */
  snprintf (tbuff, sizeof (tbuff), _("Low %s"), bpmstr);
  confuiMakeItemSpinboxNum (gui, dvbox, szgrp, szgrpC, tbuff,
      CONFUI_WIDGET_DANCE_MPM_LOW, -1, 10, 500, 0,
      confuiDanceSpinboxLowMPMChg);
  gui->uiitem [CONFUI_WIDGET_DANCE_MPM_LOW].danceitemidx = DANCE_MPM_LOW;

  /* CONTEXT: configuration: dances: high BPM (or MPM) setting */
  snprintf (tbuff, sizeof (tbuff), _("High %s"), bpmstr);
  confuiMakeItemSpinboxNum (gui, dvbox, szgrp, szgrpC, tbuff,
      CONFUI_WIDGET_DANCE_MPM_HIGH, -1, 10, 500, 0,
      confuiDanceSpinboxHighMPMChg);
  gui->uiitem [CONFUI_WIDGET_DANCE_MPM_HIGH].danceitemidx = DANCE_MPM_HIGH;

  /* CONTEXT: configuration: dances: time signature for the dance */
  confuiMakeItemSpinboxText (gui, dvbox, szgrp, szgrpC, _("Time Signature"),
      CONFUI_SPINBOX_DANCE_TIME_SIG, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxTimeSigChg);
  gui->uiitem [CONFUI_SPINBOX_DANCE_TIME_SIG].danceitemidx = DANCE_TIMESIG;

  uivlSetSelectChgCallback (gui->tables [CONFUI_ID_DANCE].uivl,
      confuiDanceSelect, gui);
#endif

  gui->inchange = false;

  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  confuiDanceSelect (gui, uivl, 0, CONFUI_DANCE_COL_DANCE);

  uiwcontFree (dvbox);
  uiwcontFree (vbox);
  uiwcontFree (hbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpB);
  uiwcontFree (szgrpC);

  logProcEnd ("");
}

void
confuiDanceSelectLoadValues (confuigui_t *gui, ilistidx_t dkey)
{
  dance_t         *dances;
  const char      *sval;
  slist_t         *slist;
  datafileconv_t  conv;
  int             widx;
  nlistidx_t      num;
  int             timesig;
  uivirtlist_t    *uivl;
  int32_t         rownum;
  char            tstr [MAXPATHLEN];


  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  rownum = uivlGetCurrSelection (uivl);

  /* the dance key must be saved because it gets changed before the */
  /* 'changed' callbacks get called */
  /* this occurs when typing into a field, then selecting a new dance */
  gui->dancedkey = uivlGetRowColumnNum (uivl, rownum, CONFUI_DANCE_COL_DANCE_KEY);

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  sval = danceGetStr (dances, dkey, DANCE_DANCE);
  widx = CONFUI_ENTRY_DANCE_DANCE;
  uiEntrySetValue (gui->uiitem [widx].uiwidgetp, sval);
  /* because the same entry field is used when switching dances, */
  /* and there is a validation timer running, */
  /* the validation timer must be cleared */
  /* the entry field does not need to be validated when being loaded */
  /* this applies to the dance, tags and announcement */
  uiEntryValidateClear (gui->uiitem [widx].uiwidgetp);

  slist = danceGetList (dances, dkey, DANCE_TAGS);
  conv.list = slist;
  conv.invt = VALUE_LIST;
  convTextList (&conv);
  sval = conv.strval;
  widx = CONFUI_ENTRY_DANCE_TAGS;
  uiEntrySetValue (gui->uiitem [widx].uiwidgetp, sval);
  dataFree (conv.strval);
  uiEntryValidateClear (gui->uiitem [widx].uiwidgetp);

  timesig = danceGetTimeSignature (dkey);

  sval = danceGetStr (dances, dkey, DANCE_ANNOUNCE);
  widx = CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT;
  stpecpy (tstr, tstr + sizeof (tstr), sval);
  pathDisplayPath (tstr, strlen (tstr));
  uiEntrySetValue (gui->uiitem [widx].uiwidgetp, tstr);
  uiEntryValidateClear (gui->uiitem [widx].uiwidgetp);

  num = danceGetNum (dances, dkey, DANCE_MPM_HIGH);
  widx = CONFUI_WIDGET_DANCE_MPM_HIGH;
  num = danceConvertMPMtoBPM (dkey, num);
  uiSpinboxSetValue (gui->uiitem [widx].uiwidgetp, num);

  num = danceGetNum (dances, dkey, DANCE_MPM_LOW);
  widx = CONFUI_WIDGET_DANCE_MPM_LOW;
  num = danceConvertMPMtoBPM (dkey, num);
  uiSpinboxSetValue (gui->uiitem [widx].uiwidgetp, num);

  num = danceGetNum (dances, dkey, DANCE_SPEED);
  widx = CONFUI_SPINBOX_DANCE_SPEED;
  uiSpinboxTextSetValue (gui->uiitem [widx].uiwidgetp, num);

  widx = CONFUI_SPINBOX_DANCE_TIME_SIG;
  uiSpinboxTextSetValue (gui->uiitem [widx].uiwidgetp, timesig);

  num = danceGetNum (dances, dkey, DANCE_TYPE);
  widx = CONFUI_SPINBOX_DANCE_TYPE;
  uiSpinboxTextSetValue (gui->uiitem [widx].uiwidgetp, num);
}

void
confuiDanceSearchSelect (confuigui_t *gui, ilistidx_t dkey)
{
  dance_t       *dances;
  slist_t       *dancelist;
  const char    *dancedisp;
  int32_t       idx;
  uivirtlist_t  *uivl;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancelist = danceGetDanceList (dances);
  dancedisp = danceGetStr (dances, dkey, DANCE_DANCE);
  idx = slistGetIdx (dancelist, dancedisp);
  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  uivlSetSelection (uivl, idx);
}

/* internal routines */

static void
confuiCreateDanceTable (confuigui_t *gui)
{
  dance_t           *dances;
  uivirtlist_t      *uivl;
  ilistidx_t        count;


  logProcBegin ();

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  uivlSetDarkBackground (uivl);
  uivlSetNumColumns (uivl, CONFUI_DANCE_COL_MAX);
  uivlMakeColumn (uivl, "dance", CONFUI_DANCE_COL_DANCE, VL_TYPE_LABEL);
  uivlSetColumnGrow (uivl, CONFUI_DANCE_COL_DANCE, VL_COL_WIDTH_GROW_ONLY);
  uivlMakeColumn (uivl, "dkey", CONFUI_DANCE_COL_DANCE_KEY, VL_TYPE_INTERNAL_NUMERIC);
  count = danceGetCount (dances);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_AUDIOSRC].currcount = count;
  uivlSetRowFillCallback (uivl, confuiDanceFillRow, gui);
  uivlDisplay (uivl);

  logProcEnd ("");
}

static int
confuiDanceEntryDanceChg (uiwcont_t *entry, const char *label, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_DANCE_DANCE);
}

static int
confuiDanceEntryTagsChg (uiwcont_t *entry, const char *label, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_DANCE_TAGS);
}

static int
confuiDanceEntryAnnouncementChg (uiwcont_t *entry, const char *label, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT);
}

static int
confuiDanceEntryChg (uiwcont_t *entry, void *udata, int widx)
{
  confuigui_t     *gui = udata;
  const char      *str = NULL;
  uivirtlist_t    *uivl = NULL;
  int             count = 0;
  dance_t         *dances;
  ilistidx_t      dkey;
  nlistidx_t      itemidx;
  datafileconv_t  conv;
  int             entryrc = UIENTRY_ERROR;

  logProcBegin ();
  if (gui->inchange) {
    logProcEnd ("in-dance-select");
    return UIENTRY_OK;
  }
  if (gui->tablecurr != CONFUI_ID_AUDIOSRC) {
    logProcEnd ("not-table-dance");
    return UIENTRY_OK;
  }

  /* note that in gtk this is a constant pointer to the current value */
  /* and the value changes due to the validation */
  str = uiEntryGetValue (entry);
  if (str == NULL) {
    logProcEnd ("null-string");
    return UIENTRY_OK;
  }

  itemidx = gui->uiitem [widx].danceitemidx;

  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  count = uivlSelectionCount (uivl);
  if (count != 1) {
    logProcEnd ("no-selection");
    return UIENTRY_OK;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dkey = gui->dancedkey;

  if (widx == CONFUI_ENTRY_DANCE_DANCE) {
    danceSetStr (dances, dkey, itemidx, str);
    uivlPopulate (uivl);
    entryrc = UIENTRY_OK;
  }
  if (widx == CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT) {
    entryrc = confuiDanceValidateAnnouncement (entry, gui);
    if (entryrc == UIENTRY_OK) {
      char    nstr [MAXPATHLEN];

      /* save the normalized version */
      stpecpy (nstr, nstr + sizeof (nstr), str);
      pathNormalizePath (nstr, strlen (nstr));
      danceSetStr (dances, dkey, itemidx, nstr);
    }
  }
  if (widx == CONFUI_ENTRY_DANCE_TAGS) {
    slist_t *slist;

    conv.str = str;
    conv.invt = VALUE_STR;
    convTextList (&conv);
    slist = conv.list;
    danceSetList (dances, dkey, itemidx, slist);
    entryrc = UIENTRY_OK;
  }
  if (entryrc == UIENTRY_OK) {
    gui->tables [gui->tablecurr].changed = true;
  }

  logProcEnd ("");
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
confuiDanceSpinboxLowMPMChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_WIDGET_DANCE_MPM_LOW);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxHighMPMChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_WIDGET_DANCE_MPM_HIGH);
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
  uivirtlist_t    *uivl = NULL;
  int             count = 0;
  int32_t         nval = 0;
  dance_t         *dances;
  ilistidx_t      dkey;
  nlistidx_t      itemidx;

  logProcBegin ();
  if (gui->inchange) {
    logProcEnd ("in-dance-select");
    return;
  }
  if (gui->tablecurr != CONFUI_ID_AUDIOSRC) {
    logProcEnd ("not-table-dance");
    return;
  }

  itemidx = gui->uiitem [widx].danceitemidx;

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

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dkey = gui->dancedkey;
  if (itemidx == DANCE_MPM_HIGH || itemidx == DANCE_MPM_LOW) {
    nval = danceConvertBPMtoMPM (dkey, nval, DANCE_NO_FORCE);
  }
  danceSetNum (dances, dkey, itemidx, nval);
  gui->tables [gui->tablecurr].changed = true;
  logProcEnd ("");
}

static int
confuiDanceValidateAnnouncement (uiwcont_t *entry, confuigui_t *gui)
{
  int         rc = UIENTRY_ERROR;
  const char  *fn;
  char        nfn [MAXPATHLEN];

  logProcBegin ();
  if (gui->inchange) {
    logProcEnd ("in-dance-select");
    return UIENTRY_OK;
  }

  gui->inchange = true;

  fn = uiEntryGetValue (entry);
  if (fn == NULL) {
    logProcEnd ("bad-fn");
    return UIENTRY_ERROR;
  }

  stpecpy (nfn, nfn + sizeof (nfn), fn);
  pathNormalizePath (nfn, sizeof (nfn));

  if (*nfn == '\0') {
    rc = UIENTRY_OK;
  } else {
    const char  *rfn;
    char        ffn [MAXPATHLEN];

    rfn = audiosrcRelativePath (nfn, 0);
    audiosrcFullPath (nfn, ffn, sizeof (ffn), NULL, 0);

    if (fileopFileExists (ffn)) {
      if (strcmp (rfn, nfn) != 0) {
        pathDisplayPath (nfn, strlen (nfn));
        uiEntrySetValue (entry, rfn);
      }
      rc = UIENTRY_OK;
    }
  }

  /* this validation routine gets called at most any time, but */
  /* the changed flag should only be set for the edit dance tab */
  if (rc == UIENTRY_OK && gui->tablecurr == CONFUI_ID_AUDIOSRC) {
    gui->tables [gui->tablecurr].changed = true;
  }

  gui->inchange = false;
  logProcEnd ("");
  return rc;
}

static void
confuiDanceSave (confuigui_t *gui)
{
  dance_t   *dances;

  logProcBegin ();

  if (gui->tables [CONFUI_ID_AUDIOSRC].changed == false) {
    logProcEnd ("not-changed");
    return;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  /* the data is already saved in the dance list; just re-use it */
  danceSave (dances, NULL, -1);
  logProcEnd ("");
}

static void
confuiLoadDanceTypeList (confuigui_t *gui)
{
  nlist_t       *tlist = NULL;
  nlist_t       *llist = NULL;
  dnctype_t     *dnctypes;
  slistidx_t    iteridx;
  const char    *key;
  int           count;

  logProcBegin ();

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
  logProcEnd ("");
}

static void
confuiDanceFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  confuigui_t *gui = udata;
  dance_t     *dances;
  slist_t     *dancelist;
  const char  *dancedisp;
  slistidx_t  dkey;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  /* dancelist has the correct display order */
  dancelist = danceGetDanceList (dances);
  dkey = slistGetNumByIdx (dancelist, rownum);
  if (dkey == LIST_VALUE_INVALID) {
    return;
  }

  dancedisp = danceGetStr (dances, dkey, DANCE_DANCE);
  uivlSetRowColumnStr (gui->tables [CONFUI_ID_AUDIOSRC].uivl, rownum,
      CONFUI_DANCE_COL_DANCE, dancedisp);
  uivlSetRowColumnNum (gui->tables [CONFUI_ID_AUDIOSRC].uivl, rownum,
      CONFUI_DANCE_COL_DANCE_KEY, dkey);
}

static void
confuiDanceSelect (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx)
{
  confuigui_t   *gui = udata;
  uivirtlist_t  *uivl;
  ilistidx_t    dkey;

  if (gui->inchange) {
    logProcEnd ("in-dance-select");
    return;
  }

  gui->inchange = true;
  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;
  if (uivl == NULL) {
    return;
  }

  dkey = uivlGetRowColumnNum (uivl, rownum, CONFUI_DANCE_COL_DANCE_KEY);
  confuiDanceSelectLoadValues (gui, dkey);
  gui->inchange = false;
}

static void
confuiDanceRemove (confuigui_t *gui, ilistidx_t rowidx)
{
  uivirtlist_t  *uivl;
  ilistidx_t    dkey;
  dance_t       *dances;
  ilistidx_t    count;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;

  dkey = uivlGetRowColumnNum (uivl, rowidx, CONFUI_DANCE_COL_DANCE_KEY);
  danceDelete (dances, dkey);
  danceSave (dances, NULL, -1);
  count = danceGetCount (dances);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_RATINGS].currcount = count;
  uivlPopulate (uivl);
  dkey = uivlGetRowColumnNum (uivl, rowidx, CONFUI_DANCE_COL_DANCE_KEY);
  confuiDanceSelectLoadValues (gui, dkey);
}

static void
confuiDanceAdd (confuigui_t *gui)
{
  dance_t       *dances;
  ilistidx_t    dkey;
  uivirtlist_t  *uivl;
  ilistidx_t    count;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  uivl = gui->tables [CONFUI_ID_AUDIOSRC].uivl;

  /* CONTEXT: configuration: dance name that is set when adding a new dance */
  dkey = danceAdd (dances, _("New Dance"));
  danceSave (dances, NULL, -1);
  count = danceGetCount (dances);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_RATINGS].currcount = count;
  uivlPopulate (uivl);
  confuiDanceSearchSelect (gui, dkey);
}
