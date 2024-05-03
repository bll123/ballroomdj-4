/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjstring.h"
#include "callback.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "playlist.h"
#include "sysvars.h"
#include "ui.h"
#include "uiexp.h"
#include "uiplaylist.h"
#include "uiutils.h"
#include "validate.h"

enum {
  UIEXP_CB_DIALOG,
  UIEXP_CB_TARGET,
  UIEXP_CB_SEL,
  UIEXP_CB_MAX,
};

enum {
  UIEXP_W_STATUS_MSG,
  UIEXP_W_ERROR_MSG,
  UIEXP_W_DIALOG,
  UIEXP_W_TARGET,
  UIEXP_W_NEWNAME,
  UIEXP_W_TGT_BUTTON,
  UIEXP_W_MAX,
};

typedef struct uiexp {
  uiwcont_t         *wcont [UIEXP_W_MAX];
  uiplaylist_t      *uiplaylist;
  callback_t        *responsecb;
  uiwcont_t         *parentwin;
  nlist_t           *options;
  callback_t        *callbacks [UIEXP_CB_MAX];
  bool              isactive : 1;
} uiexp_t;

/* export playlist */
static void   uiexpCreateDialog (uiexp_t *uiexp);
static bool   uiexpTargetDialog (void *udata);
static void   uiexpInitDisplay (uiexp_t *uiexp);
static bool   uiexpResponseHandler (void *udata, long responseid);
static void   uiexpFreeDialog (uiexp_t *uiexp);
static int    uiexpValidateTarget (uiwcont_t *entry, void *udata);
static bool   uiexpSelectHandler (void *udata, long idx);
static int    uiexpValidateNewName (uiwcont_t *entry, void *udata);

uiexp_t *
uiexpInit (uiwcont_t *windowp, nlist_t *opts)
{
  uiexp_t  *uiexp;

  uiexp = mdmalloc (sizeof (uiexp_t));
  for (int j = 0; j < UIEXP_W_MAX; ++j) {
    uiexp->wcont [j] = NULL;
  }
  uiexp->uiplaylist = NULL;
  uiexp->responsecb = NULL;
  uiexp->parentwin = windowp;
  uiexp->options = opts;
  for (int i = 0; i < UIEXP_CB_MAX; ++i) {
    uiexp->callbacks [i] = NULL;
  }
  uiexp->isactive = false;

  uiexp->callbacks [UIEXP_CB_DIALOG] = callbackInitLong (
      uiexpResponseHandler, uiexp);
  uiexp->callbacks [UIEXP_CB_TARGET] = callbackInit (
      uiexpTargetDialog, uiexp, NULL);
  uiexp->callbacks [UIEXP_CB_SEL] = callbackInitLong (
      uiexpSelectHandler, uiexp);

  return uiexp;
}

void
uiexpFree (uiexp_t *uiexp)
{
  if (uiexp == NULL) {
    return;
  }

  for (int i = 0; i < UIEXP_CB_MAX; ++i) {
    callbackFree (uiexp->callbacks [i]);
  }
  uiexpFreeDialog (uiexp);
  mdfree (uiexp);
}

void
uiexpSetResponseCallback (uiexp_t *uiexp, callback_t *uicb)
{
  if (uiexp == NULL) {
    return;
  }
  uiexp->responsecb = uicb;
}

bool
uiexpDialog (uiexp_t *uiexp)
{
  int         x, y;

  if (uiexp == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uiexpDialog");
  uiexpCreateDialog (uiexp);
  uiexpInitDisplay (uiexp);
  uiDialogShow (uiexp->wcont [UIEXP_W_DIALOG]);
  uiexp->isactive = true;

  x = nlistGetNum (uiexp->options, EXP_PL_POSITION_X);
  y = nlistGetNum (uiexp->options, EXP_PL_POSITION_Y);
  uiWindowMove (uiexp->wcont [UIEXP_W_DIALOG], x, y, -1);
  logProcEnd (LOG_PROC, "uiexpDialog", "");
  return UICB_CONT;
}

void
uiexpDialogClear (uiexp_t *uiexp)
{
  if (uiexp == NULL) {
    return;
  }

  uiWidgetHide (uiexp->wcont [UIEXP_W_DIALOG]);
}

/* delayed entry validation for the audio file needs to be run */
void
uiexpProcess (uiexp_t *uiexp)
{
  if (uiexp == NULL) {
    return;
  }
  if (! uiexp->isactive) {
    return;
  }

  uiEntryValidate (
    uiexp->wcont [UIEXP_W_TARGET], false);
  uiEntryValidate (
    uiexp->wcont [UIEXP_W_NEWNAME], false);
}

char *
uiexpGetDir (uiexp_t *uiexp)
{
  const char  *tdir;
  char        *dir;

  if (uiexp == NULL) {
    return NULL;
  }

  tdir = uiEntryGetValue (uiexp->wcont [UIEXP_W_TARGET]);
  dir = mdstrdup (tdir);
  return dir;
}

const char *
uiexpGetNewName (uiexp_t *uiexp)
{
  const char  *newname;

  if (uiexp == NULL) {
    return NULL;
  }

  newname = uiEntryGetValue (uiexp->wcont [UIEXP_W_NEWNAME]);
  return newname;
}

void
uiexpUpdateStatus (uiexp_t *uiexp, int count, int tot)
{
  if (uiexp == NULL) {
    return;
  }

  uiutilsProgressStatus (
      uiexp->wcont [UIEXP_W_STATUS_MSG],
      count, tot);
}

/* internal routines */

static void
uiexpCreateDialog (uiexp_t *uiexp)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp = NULL;
  uiwcont_t     *szgrp;  // labels
  const char    *buttontext;
  char          tbuff [100];
  const char    *odir = NULL;

  logProcBegin (LOG_PROC, "uiexpCreateDialog");

  if (uiexp == NULL) {
    return;
  }

  if (uiexp->wcont [UIEXP_W_DIALOG] != NULL) {
    return;
  }

  buttontext = "";
  *tbuff = '\0';
  /* CONTEXT: export playlist dialog: export button */
  buttontext = _("Export");
  /* CONTEXT: export for ballroomdj: title of dialog */
  snprintf (tbuff, sizeof (tbuff), _("Export for %s"), BDJ4_NAME);

  szgrp = uiCreateSizeGroupHoriz ();

  uiexp->wcont [UIEXP_W_DIALOG] =
      uiCreateDialog (uiexp->parentwin,
      uiexp->callbacks [UIEXP_CB_DIALOG],
      tbuff,
      /* CONTEXT: export/import bdj4 dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      buttontext,
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiDialogPackInDialog (
      uiexp->wcont [UIEXP_W_DIALOG], vbox);

  /* status msg */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiexp->wcont [UIEXP_W_STATUS_MSG] = uiwidgetp;

  /* error msg */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, ERROR_CLASS);
  uiexp->wcont [UIEXP_W_ERROR_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* target folder */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: export playlist: export folder location */
      _("Export to"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (50, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiexp->wcont [UIEXP_W_TARGET] = uiwidgetp;

  odir = nlistGetStr (uiexp->options, MANAGE_EXP_BDJ4_DIR);
  if (odir == NULL) {
    odir = sysvarsGetStr (SV_HOME);
  }
  strlcpy (tbuff, odir, sizeof (tbuff));
  pathDisplayPath (tbuff, sizeof (tbuff));
  uiEntrySetValue (uiexp->wcont [UIEXP_W_TARGET], tbuff);
  uiEntrySetValidate (uiwidgetp,
      uiexpValidateTarget, uiexp, UIENTRY_DELAYED);

  uiwidgetp = uiCreateButton (
      uiexp->callbacks [UIEXP_CB_TARGET],
      "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiexp->wcont [UIEXP_W_TGT_BUTTON] = uiwidgetp;

  uiwcontFree (hbox);

  /* playlist selector */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: export/import bdj4: playlist: select the song list */
  uiwidgetp = uiCreateColonLabel (_("Song List"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiexp->uiplaylist = uiplaylistCreate (
      uiexp->wcont [UIEXP_W_DIALOG],
      hbox, PL_LIST_NORMAL);
  uiplaylistSetSelectCallback (uiexp->uiplaylist,
      uiexp->callbacks [UIEXP_CB_SEL]);

  uiwcontFree (hbox);

  /* new name */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: export/import bdj4: new song list name */
      _("New Song List Name"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (30, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiBoxPackStart (hbox, uiwidgetp);
  uiexp->wcont [UIEXP_W_NEWNAME] = uiwidgetp;

  uiEntrySetValidate (uiwidgetp,
      uiexpValidateNewName, uiexp, UIENTRY_IMMEDIATE);

  uiwcontFree (hbox);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  logProcEnd (LOG_PROC, "uiexpCreateDialog", "");
}

static bool
uiexpTargetDialog (void *udata)
{
  uiexp_t  *uiexp = udata;
  uiselect_t  *selectdata;
  const char  *odir = NULL;
  char        *dir = NULL;

  if (uiexp == NULL) {
    return UICB_STOP;
  }

  odir = uiEntryGetValue (uiexp->wcont [UIEXP_W_TARGET]);
  selectdata = uiSelectInit (uiexp->parentwin,
      /* CONTEXT: export/import bdj4 folder selection dialog: window title */
      _("Select Folder"), odir, NULL, NULL, NULL);

  dir = uiSelectDirDialog (selectdata);
  if (dir != NULL) {
    /* the validation process will be called */
    uiEntrySetValue (uiexp->wcont [UIEXP_W_TARGET], dir);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", dir);
    mdfree (dir);   // allocated by gtk
  }
  uiSelectFree (selectdata);

  return UICB_CONT;
}


static void
uiexpInitDisplay (uiexp_t *uiexp)
{
  if (uiexp == NULL) {
    return;
  }
}

static bool
uiexpResponseHandler (void *udata, long responseid)
{
  uiexp_t  *uiexp = udata;
  int             x, y, ws;

  uiWindowGetPosition (
      uiexp->wcont [UIEXP_W_DIALOG], &x, &y, &ws);
  nlistSetNum (uiexp->options, EXP_PL_POSITION_X, x);
  nlistSetNum (uiexp->options, EXP_PL_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: del window");
      uiexpFreeDialog (uiexp);
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: close window");
      uiWidgetHide (uiexp->wcont [UIEXP_W_DIALOG]);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: apply");
      uiLabelSetText (
          uiexp->wcont [UIEXP_W_STATUS_MSG],
          /* CONTEXT: please wait... status message */
          _("Please wait\xe2\x80\xa6"));
      /* do not close or hide the dialog; it will stay active and */
      /* the status message will be updated */
      if (uiexp->responsecb != NULL) {
        callbackHandler (uiexp->responsecb);
      }
      break;
    }
  }

  uiexp->isactive = false;
  return UICB_CONT;
}

static void
uiexpFreeDialog (uiexp_t *uiexp)
{
  for (int j = 0; j < UIEXP_W_MAX; ++j) {
    uiwcontFree (uiexp->wcont [j]);
    uiexp->wcont [j] = NULL;
  }
  uiwcontFree (uiexp->wcont [UIEXP_W_TGT_BUTTON]);
  uiexp->wcont [UIEXP_W_TGT_BUTTON] = NULL;
  uiplaylistFree (uiexp->uiplaylist);
  uiexp->uiplaylist = NULL;
}

static int
uiexpValidateTarget (uiwcont_t *entry, void *udata)
{
  uiexp_t  *uiexp = udata;
  const char  *str;
  char        tbuff [MAXPATHLEN];

  uiLabelSetText (
      uiexp->wcont [UIEXP_W_ERROR_MSG], "");

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
  if (! *str || ! fileopIsDirectory (str) || ! fileopIsDirectory (tbuff)) {
    if (*str) {
      uiLabelSetText (uiexp->wcont [UIEXP_W_ERROR_MSG],
          /* CONTEXT: export/import bdj4: invalid target folder */
          _("Invalid Folder"));
    }
    return UIENTRY_ERROR;
  }

  return UIENTRY_OK;
}


static bool
uiexpSelectHandler (void *udata, long idx)
{
  uiexp_t  *uiexp = udata;
  const char  *str;

  str = uiplaylistGetValue (uiexp->uiplaylist);
  uiEntrySetValue (uiexp->wcont [UIEXP_W_NEWNAME], str);
  return UICB_CONT;
}

static int
uiexpValidateNewName (uiwcont_t *entry, void *udata)
{
  uiexp_t  *uiexp = udata;
  uiwcont_t   *statusMsg = NULL;
  uiwcont_t   *errorMsg = NULL;
  int         rc = UIENTRY_ERROR;
  const char  *str;
  char        fn [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];

  statusMsg = uiexp->wcont [UIEXP_W_STATUS_MSG];
  errorMsg = uiexp->wcont [UIEXP_W_ERROR_MSG];
  uiLabelSetText (statusMsg, "");
  uiLabelSetText (errorMsg, "");
  rc = uiutilsValidatePlaylistName (entry, errorMsg);

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
