/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "asconf.h"
#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjstring.h"
#include "callback.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "playlist.h"
#include "slist.h"
#include "sysvars.h"
#include "ui.h"
#include "uidd.h"
#include "uiimppl.h"
#include "uiutils.h"
#include "validate.h"

enum {
  UIIMPPL_CB_DIALOG,
  UIIMPPL_CB_SEL,
  UIIMPPL_CB_SOURCE_TYPE,
  UIIMPPL_CB_TARGET,
  UIIMPPL_CB_TYPE_SEL,
  UIIMPPL_CB_MAX,
};

enum {
  UIIMPPL_W_STATUS_MSG,
  UIIMPPL_W_ERROR_MSG,
  UIIMPPL_W_DIALOG,
  UIIMPPL_W_TARGET,
  UIIMPPL_W_URI_LABEL,
  UIIMPPL_W_NEWNAME,
  UIIMPPL_W_IMP_TYPE,
  UIIMPPL_W_MAX,
};

typedef struct uiimppl {
  uiwcont_t         *parentwin;
  uiwcont_t         *wcont [UIIMPPL_W_MAX];
  asconf_t          *asconf;
  uiwcont_t         *targetButton;
  callback_t        *responsecb;
  uidd_t            *plselect;
  nlist_t           *options;
  nlist_t           *aslist;
  slist_t           *astypes;
  ilist_t           *plnames;
  callback_t        *callbacks [UIIMPPL_CB_MAX];
  size_t            asmaxwidth;
  int               asidx;
  int               imptype;
  bool              isactive;
} uiimppl_t;

/* import playlist */
static void   uiimpplCreateDialog (uiimppl_t *uiimppl);
static bool   uiimpplTargetDialog (void *udata);
static void   uiimpplInitDisplay (uiimppl_t *uiimppl);
static bool   uiimpplResponseHandler (void *udata, int32_t responseid);
static int    uiimpplValidateTarget (uiwcont_t *entry, const char *label, void *udata);
static int32_t uiimpplSelectHandler (void *udata, const char *str);
static int    uiimpplValidateNewName (uiwcont_t *entry, const char *label, void *udata);
static bool uiimpplImportTypeCallback (void *udata);

uiimppl_t *
uiimpplInit (uiwcont_t *windowp, nlist_t *opts)
{
  uiimppl_t   *uiimppl;
  const char  *asnm;
  size_t      len;
  ilistidx_t  asiteridx;
  ilistidx_t  key;
  slistidx_t  titeridx;
  int         count;

  uiimppl = mdmalloc (sizeof (uiimppl_t));
  for (int j = 0; j < UIIMPPL_W_MAX; ++j) {
    uiimppl->wcont [j] = NULL;
  }
  uiimppl->targetButton = NULL;
  uiimppl->responsecb = NULL;
  uiimppl->parentwin = windowp;
  uiimppl->options = opts;
  for (int i = 0; i < UIIMPPL_CB_MAX; ++i) {
    uiimppl->callbacks [i] = NULL;
  }
  uiimppl->isactive = false;
  uiimppl->aslist = NULL;
  uiimppl->asidx = 0;
  uiimppl->plnames = NULL;

  uiimppl->callbacks [UIIMPPL_CB_DIALOG] = callbackInitI (
      uiimpplResponseHandler, uiimppl);
  uiimppl->callbacks [UIIMPPL_CB_TARGET] = callbackInit (
      uiimpplTargetDialog, uiimppl, NULL);
  uiimppl->callbacks [UIIMPPL_CB_SEL] = callbackInitS (
      uiimpplSelectHandler, uiimppl);
  uiimppl->callbacks [UIIMPPL_CB_TYPE_SEL] = callbackInit (
      uiimpplImportTypeCallback, uiimppl, NULL);

  uiimppl->asconf = asconfAlloc ();

  count = 0;
  uiimppl->astypes = slistAlloc ("aslist", LIST_UNORDERED, NULL);
  slistSetNum (uiimppl->astypes, "M3U/XSPF/JSPF", AUDIOSRC_TYPE_FILE);

  asconfStartIterator (uiimppl->asconf, &asiteridx);
  while ((key = asconfIterate (uiimppl->asconf, &asiteridx)) >= 0) {
    int   type;

    type = asconfGetNum (uiimppl->asconf, key, ASCONF_TYPE);
    if (type == AUDIOSRC_TYPE_NONE || type == AUDIOSRC_TYPE_FILE) {
      continue;
    }
    asnm = asconfGetStr (uiimppl->asconf, key, ASCONF_NAME);
    len = strlen (asnm);
    if (len > uiimppl->asmaxwidth) {
      uiimppl->asmaxwidth = len;
    }
    slistSetNum (uiimppl->astypes, asnm, type);
  }
  slistSort (uiimppl->astypes);

  uiimppl->aslist = nlistAlloc ("aslist", LIST_ORDERED, NULL);
  count = 0;
  slistStartIterator (uiimppl->astypes, &titeridx);
  while ((asnm = slistIterateKey (uiimppl->astypes, &titeridx)) != NULL) {
    if (strncmp (asnm, "M3U", 3) == 0) {
      uiimppl->asidx = count;
    }
    len = strlen (asnm);
    uiimppl->asmaxwidth = len;
    nlistSetStr (uiimppl->aslist, count, asnm);
    ++count;
  }

  return uiimppl;
}

void
uiimpplFree (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return;
  }

  for (int j = 0; j < UIIMPPL_W_MAX; ++j) {
    uiwcontFree (uiimppl->wcont [j]);
    uiimppl->wcont [j] = NULL;
  }
  uiwcontFree (uiimppl->targetButton);
  uiimppl->targetButton = NULL;
  for (int i = 0; i < UIIMPPL_CB_MAX; ++i) {
    callbackFree (uiimppl->callbacks [i]);
  }
  asconfFree (uiimppl->asconf);
  mdfree (uiimppl);
}

void
uiimpplSetResponseCallback (uiimppl_t *uiimppl, callback_t *uicb)
{
  if (uiimppl == NULL) {
    return;
  }
  uiimppl->responsecb = uicb;
}

bool
uiimpplDialog (uiimppl_t *uiimppl)
{
  int         x, y;

  if (uiimppl == NULL) {
    return UICB_STOP;
  }

  logProcBegin ();
  uiimpplCreateDialog (uiimppl);
  uiimpplInitDisplay (uiimppl);
  uiDialogShow (uiimppl->wcont [UIIMPPL_W_DIALOG]);
  uiimppl->isactive = true;

  x = nlistGetNum (uiimppl->options, EXP_IMP_BDJ4_POSITION_X);
  y = nlistGetNum (uiimppl->options, EXP_IMP_BDJ4_POSITION_Y);
  uiWindowMove (uiimppl->wcont [UIIMPPL_W_DIALOG], x, y, -1);
  logProcEnd ("");
  return UICB_CONT;
}

void
uiimpplDialogClear (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return;
  }

  uiWidgetHide (uiimppl->wcont [UIIMPPL_W_DIALOG]);
}

/* delayed entry validation for the audio file needs to be run */
void
uiimpplProcess (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return;
  }
  if (! uiimppl->isactive) {
    return;
  }

  uiEntryValidate (
    uiimppl->wcont [UIIMPPL_W_TARGET], false);
  uiEntryValidate (
    uiimppl->wcont [UIIMPPL_W_NEWNAME], false);
}

char *
uiimpplGetDir (uiimppl_t *uiimppl)
{
  const char  *tdir;
  char        *dir;

  if (uiimppl == NULL) {
    return NULL;
  }

  tdir = uiEntryGetValue (uiimppl->wcont [UIIMPPL_W_TARGET]);
  dir = mdstrdup (tdir);
  return dir;
}

const char *
uiimpplGetPlaylist (uiimppl_t *uiimppl)
{
  const char  *plname = NULL;

  if (uiimppl == NULL) {
    return NULL;
  }

//  plname = uiplaylistGetKey (uiimppl->uiplaylist);
  return plname;
}

const char *
uiimpplGetNewName (uiimppl_t *uiimppl)
{
  const char  *newname;

  if (uiimppl == NULL) {
    return NULL;
  }

  newname = uiEntryGetValue (uiimppl->wcont [UIIMPPL_W_NEWNAME]);
  return newname;
}

void
uiimpplUpdateStatus (uiimppl_t *uiimppl, int count, int tot)
{
  if (uiimppl == NULL) {
    return;
  }

  uiutilsProgressStatus (
      uiimppl->wcont [UIIMPPL_W_STATUS_MSG],
      count, tot);
}

/* internal routines */

static void
uiimpplCreateDialog (uiimppl_t *uiimppl)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp = NULL;
  uiwcont_t     *szgrp;  // labels
  const char    *odir = NULL;
  char          tbuff [MAXPATHLEN];

  logProcBegin ();

  if (uiimppl == NULL) {
    return;
  }

  if (uiimppl->wcont [UIIMPPL_W_DIALOG] != NULL) {
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();

  uiimppl->wcont [UIIMPPL_W_DIALOG] =
      uiCreateDialog (uiimppl->parentwin,
      uiimppl->callbacks [UIIMPPL_CB_DIALOG],
      /* CONTEXT: import playlist: title of dialog */
      _("Import Playlist"),
      /* CONTEXT: import playlist dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      _("Import"),
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiDialogPackInDialog (
      uiimppl->wcont [UIIMPPL_W_DIALOG], vbox);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiWidgetSetAllMargins (vbox, 4);

  /* status msg */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
  uiimppl->wcont [UIIMPPL_W_STATUS_MSG] = uiwidgetp;

  /* error msg */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, ERROR_CLASS);
  uiimppl->wcont [UIIMPPL_W_ERROR_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* type */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: import playlist: import-from selection */
      _("Import From"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiSpinboxTextCreate (uiimppl);
  uiSpinboxTextSet (uiwidgetp, 0, nlistGetCount (uiimppl->aslist),
      uiimppl->asmaxwidth, uiimppl->aslist, NULL, NULL);
  uiSpinboxTextSetValue (uiwidgetp, uiimppl->asidx);
  uiSpinboxTextSetValueChangedCallback (uiwidgetp,
      uiimppl->callbacks [UIIMPPL_CB_TYPE_SEL]);
  uiimppl->wcont [UIIMPPL_W_IMP_TYPE] = uiwidgetp;
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);

  /* target folder */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: import playlist: import location */
      _("File"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiimppl->wcont [UIIMPPL_W_URI_LABEL] = uiwidgetp;

  uiwidgetp = uiEntryInit (50, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiimppl->wcont [UIIMPPL_W_TARGET] = uiwidgetp;

  odir = nlistGetStr (uiimppl->options, MANAGE_IMP_BDJ4_DIR);
  if (odir == NULL) {
    odir = sysvarsGetStr (SV_HOME);
  }
  stpecpy (tbuff, tbuff + sizeof (tbuff), odir);
  pathDisplayPath (tbuff, sizeof (tbuff));
  uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_TARGET], tbuff);
  uiEntrySetValidate (uiwidgetp, "",
      uiimpplValidateTarget, uiimppl, UIENTRY_DELAYED);

  uiwidgetp = uiCreateButton ("imppl-folder",
      uiimppl->callbacks [UIIMPPL_CB_TARGET],
      "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiimppl->targetButton = uiwidgetp;

  uiwcontFree (hbox);

  /* playlist selector */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (_("Playlist"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiimppl->plselect = uiddCreate ("imppl-plsel", uiimppl->parentwin, hbox,
      /* CONTEXT: import playlist: select the playlist */
      DD_PACK_START, NULL, DD_LIST_TYPE_NUM, "", DD_REPLACE_TITLE,
      uiimppl->callbacks [UIIMPPL_CB_SEL]);

  uiwcontFree (hbox);

  /* new name */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: import playlist: new song list name */
      _("New Song List Name"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (30, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiBoxPackStart (hbox, uiwidgetp);
  uiimppl->wcont [UIIMPPL_W_NEWNAME] = uiwidgetp;

  /* CONTEXT: import playlist: select the song list */
  uiEntrySetValidate (uiwidgetp, _("Playlist"),
      uiimpplValidateNewName, uiimppl, UIENTRY_IMMEDIATE);

  uiwcontFree (hbox);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  uiSpinboxTextSetValueChangedCallback (uiimppl->wcont [UIIMPPL_W_IMP_TYPE],
      uiimppl->callbacks [UIIMPPL_CB_TYPE_SEL]);

  logProcEnd ("");
}

static bool
uiimpplTargetDialog (void *udata)
{
  uiimppl_t  *uiimppl = udata;
  uiselect_t  *selectdata;
  const char  *odir = NULL;
  char        *dir = NULL;

  if (uiimppl == NULL) {
    return UICB_STOP;
  }

  odir = uiEntryGetValue (uiimppl->wcont [UIIMPPL_W_TARGET]);
  selectdata = uiSelectInit (uiimppl->parentwin,
      /* CONTEXT: import playlist file selection dialog: window title */
      _("Select Folder"), odir, NULL, NULL, NULL);

  dir = uiSelectDirDialog (selectdata);
  if (dir != NULL) {
    /* the validation process will be called */
    uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_TARGET], dir);
    logMsg (LOG_DBG, LOG_IMPORTANT, "selected dir: %s", dir);
    mdfree (dir);   // allocated by gtk
  }
  uiSelectFree (selectdata);

  return UICB_CONT;
}


static void
uiimpplInitDisplay (uiimppl_t *uiimppl)
{
  if (uiimppl == NULL) {
    return;
  }
}

static bool
uiimpplResponseHandler (void *udata, int32_t responseid)
{
  uiimppl_t  *uiimppl = udata;
  int             x, y, ws;

  uiWindowGetPosition (
      uiimppl->wcont [UIIMPPL_W_DIALOG], &x, &y, &ws);
  nlistSetNum (uiimppl->options, EXP_IMP_BDJ4_POSITION_X, x);
  nlistSetNum (uiimppl->options, EXP_IMP_BDJ4_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: del window");
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: close window");
      uiWidgetHide (uiimppl->wcont [UIIMPPL_W_DIALOG]);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: apply");
      uiLabelSetText (
          uiimppl->wcont [UIIMPPL_W_STATUS_MSG],
          /* CONTEXT: please wait... status message */
          _("Please wait\xe2\x80\xa6"));
      /* do not close or hide the dialog; it will stay active and */
      /* the status message will be updated */
      if (uiimppl->responsecb != NULL) {
        callbackHandler (uiimppl->responsecb);
      }
      break;
    }
  }

  uiimppl->isactive = false;
  return UICB_CONT;
}


static int
uiimpplValidateTarget (uiwcont_t *entry, const char *label, void *udata)
{
  uiimppl_t  *uiimppl = udata;
  const char  *str;
  char        tbuff [MAXPATHLEN];

  uiLabelSetText (
      uiimppl->wcont [UIIMPPL_W_ERROR_MSG], "");

  str = uiEntryGetValue (entry);
  *tbuff = '\0';
  if (str != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%s/data", str);
  }
  pathNormalizePath (tbuff, sizeof (tbuff));

  /* validation failures:
   *   target is not set (no message displayed)
   *   target is not a directory
   *   if importing, target/data is not a directory
   */
  if (! *str || ! fileopIsDirectory (str) ||
      ! fileopIsDirectory (tbuff)) {
    if (*str) {
      uiLabelSetText (uiimppl->wcont [UIIMPPL_W_ERROR_MSG],
          /* CONTEXT: export/import bdj4: invalid target folder */
          _("Invalid Folder"));
    }
    return UIENTRY_ERROR;
  }

//  uiddSetList (uiimppl->uiplaylist, PL_LIST_DIR, tbuff);
  return UIENTRY_OK;
}

static int32_t
uiimpplSelectHandler (void *udata, const char *str)
{
  uiimppl_t  *uiimppl = udata;

  uiEntrySetValue (uiimppl->wcont [UIIMPPL_W_NEWNAME], str);
  return UICB_CONT;
}

static int
uiimpplValidateNewName (uiwcont_t *entry, const char *label, void *udata)
{
  uiimppl_t  *uiimppl = udata;
  uiwcont_t   *statusMsg = NULL;
  uiwcont_t   *errorMsg = NULL;
  int         rc = UIENTRY_ERROR;
  const char  *str;
  char        fn [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];

  statusMsg = uiimppl->wcont [UIIMPPL_W_STATUS_MSG];
  errorMsg = uiimppl->wcont [UIIMPPL_W_ERROR_MSG];
  uiLabelSetText (statusMsg, "");
  uiLabelSetText (errorMsg, "");
  rc = uiutilsValidatePlaylistName (entry, label, errorMsg);

  if (rc == UIENTRY_OK) {
    str = uiEntryGetValue (entry);
    if (*str) {
      pathbldMakePath (fn, sizeof (fn),
          str, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
      if (fileopFileExists (fn)) {
        /* CONTEXT: import from bdj4: error message if the target song list already exists */
        snprintf (tbuff, sizeof (tbuff), "%s: %s", str, _("Already Exists"));
        uiLabelSetText (statusMsg, tbuff);
      }
    }
  }

  return rc;
}

static bool
uiimpplImportTypeCallback (void *udata)
{
  uiimppl_t   *uiimppl = udata;
  char        tbuff [40];

  uiimppl->imptype = uiSpinboxTextGetValue (
      uiimppl->wcont [UIIMPPL_W_IMP_TYPE]);

  if (uiimppl->imptype == AUDIOSRC_TYPE_FILE) {
    /* CONTEXT: import playlist: import location */
    snprintf (tbuff, sizeof (tbuff), "%s:", _("File"));
  } else {
    /* CONTEXT: import playlist: import location */
    snprintf (tbuff, sizeof (tbuff), "%s:", _("URI"));
  }
  uiLabelSetText (uiimppl->wcont [UIIMPPL_W_URI_LABEL], tbuff);

  if (uiimppl->imptype != AUDIOSRC_TYPE_FILE) {
    asiter_t    *asiter;
    const char  *plnm;
    int         idx;

    idx = 0;
    ilistFree (uiimppl->plnames);
    asiter = audiosrcStartIterator (uiimppl->imptype, AS_ITER_PL_NAMES, NULL, -1);
    uiimppl->plnames = ilistAlloc ("plnames", LIST_ORDERED);
    ilistSetSize (uiimppl->plnames, audiosrcIterCount (asiter));
    while ((plnm = audiosrcIterate (asiter)) != NULL) {
fprintf (stderr, "plnm: %s\n", plnm);
      ilistSetNum (uiimppl->plnames, idx, DD_LIST_KEY_NUM, idx);
      ilistSetStr (uiimppl->plnames, idx, DD_LIST_DISP, plnm);
      ++idx;
    }
    audiosrcCleanIterator (asiter);
    uiddSetList (uiimppl->plselect, uiimppl->plnames);
  }

  return UICB_CONT;
}

