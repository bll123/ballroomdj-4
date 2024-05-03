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
#include "uiexppl.h"
#include "uiplaylist.h"
#include "uiutils.h"
#include "validate.h"

enum {
  UIEXPPL_CB_DIALOG,
  UIEXPPL_CB_TARGET,
  UIEXPPL_CB_SEL,
  UIEXPPL_CB_MAX,
};

enum {
  UIEXPPL_W_STATUS_MSG,
  UIEXPPL_W_ERROR_MSG,
  UIEXPPL_W_DIALOG,
  UIEXPPL_W_TARGET,
  UIEXPPL_W_NEWNAME,
  UIEXPPL_W_TGT_BUTTON,
  UIEXPPL_W_MAX,
};

typedef struct uiexppl {
  uiwcont_t         *wcont [UIEXPPL_W_MAX];
  uiplaylist_t      *uiplaylist;
  callback_t        *responsecb;
  uiwcont_t         *parentwin;
  nlist_t           *options;
  callback_t        *callbacks [UIEXPPL_CB_MAX];
  bool              isactive : 1;
} UIEXPPL_t;

/* export playlist */
static void   uiexpplCreateDialog (UIEXPPL_t *uiexppl);
static bool   uiexpplTargetDialog (void *udata);
static void   uiexpplInitDisplay (UIEXPPL_t *uiexppl);
static bool   uiexpplResponseHandler (void *udata, long responseid);
static void   uiexpplFreeDialog (UIEXPPL_t *uiexppl);
static int    uiexpplValidateTarget (uiwcont_t *entry, void *udata);
static bool   uiexpplSelectHandler (void *udata, long idx);
static int    uiexpplValidateNewName (uiwcont_t *entry, void *udata);

UIEXPPL_t *
uiexpplInit (uiwcont_t *windowp, nlist_t *opts)
{
  UIEXPPL_t  *uiexppl;

  uiexppl = mdmalloc (sizeof (UIEXPPL_t));
  for (int j = 0; j < UIEXPPL_W_MAX; ++j) {
    uiexppl->wcont [j] = NULL;
  }
  uiexppl->uiplaylist = NULL;
  uiexppl->responsecb = NULL;
  uiexppl->parentwin = windowp;
  uiexppl->options = opts;
  for (int i = 0; i < UIEXPPL_CB_MAX; ++i) {
    uiexppl->callbacks [i] = NULL;
  }
  uiexppl->isactive = false;

  uiexppl->callbacks [UIEXPPL_CB_DIALOG] = callbackInitLong (
      uiexpplResponseHandler, uiexppl);
  uiexppl->callbacks [UIEXPPL_CB_TARGET] = callbackInit (
      uiexpplTargetDialog, uiexppl, NULL);
  uiexppl->callbacks [UIEXPPL_CB_SEL] = callbackInitLong (
      uiexpplSelectHandler, uiexppl);

  return uiexppl;
}

void
uiexpplFree (UIEXPPL_t *uiexppl)
{
  if (uiexppl == NULL) {
    return;
  }

  for (int i = 0; i < UIEXPPL_CB_MAX; ++i) {
    callbackFree (uiexppl->callbacks [i]);
  }
  uiexpplFreeDialog (uiexppl);
  mdfree (uiexppl);
}

void
uiexpplSetResponseCallback (UIEXPPL_t *uiexppl, callback_t *uicb)
{
  if (uiexppl == NULL) {
    return;
  }
  uiexppl->responsecb = uicb;
}

bool
uiexpplDialog (UIEXPPL_t *uiexppl)
{
  int         x, y;

  if (uiexppl == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uiexpplDialog");
  uiexpplCreateDialog (uiexppl);
  uiexpplInitDisplay (uiexppl);
  uiDialogShow (uiexppl->wcont [UIEXPPL_W_DIALOG]);
  uiexppl->isactive = true;

  x = nlistGetNum (uiexppl->options, EXP_PL_POSITION_X);
  y = nlistGetNum (uiexppl->options, EXP_PL_POSITION_Y);
  uiWindowMove (uiexppl->wcont [UIEXPPL_W_DIALOG], x, y, -1);
  logProcEnd (LOG_PROC, "uiexpplDialog", "");
  return UICB_CONT;
}

void
uiexpplDialogClear (UIEXPPL_t *uiexppl)
{
  if (uiexppl == NULL) {
    return;
  }

  uiWidgetHide (uiexppl->wcont [UIEXPPL_W_DIALOG]);
}

/* delayed entry validation for the audio file needs to be run */
void
uiexpplProcess (UIEXPPL_t *uiexppl)
{
  if (uiexppl == NULL) {
    return;
  }
  if (! uiexppl->isactive) {
    return;
  }

  uiEntryValidate (
    uiexppl->wcont [UIEXPPL_W_TARGET], false);
  uiEntryValidate (
    uiexppl->wcont [UIEXPPL_W_NEWNAME], false);
}

char *
uiexpplGetDir (UIEXPPL_t *uiexppl)
{
  const char  *tdir;
  char        *dir;

  if (uiexppl == NULL) {
    return NULL;
  }

  tdir = uiEntryGetValue (uiexppl->wcont [UIEXPPL_W_TARGET]);
  dir = mdstrdup (tdir);
  return dir;
}

const char *
uiexpplGetNewName (UIEXPPL_t *uiexppl)
{
  const char  *newname;

  if (uiexppl == NULL) {
    return NULL;
  }

  newname = uiEntryGetValue (uiexppl->wcont [UIEXPPL_W_NEWNAME]);
  return newname;
}

void
uiexpplUpdateStatus (UIEXPPL_t *uiexppl, int count, int tot)
{
  if (uiexppl == NULL) {
    return;
  }

  uiutilsProgressStatus (
      uiexppl->wcont [UIEXPPL_W_STATUS_MSG],
      count, tot);
}

/* internal routines */

static void
uiexpplCreateDialog (UIEXPPL_t *uiexppl)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp = NULL;
  uiwcont_t     *szgrp;  // labels
  const char    *buttontext;
  char          tbuff [100];
  const char    *odir = NULL;

  logProcBegin (LOG_PROC, "uiexpplCreateDialog");

  if (uiexppl == NULL) {
    return;
  }

  if (uiexppl->wcont [UIEXPPL_W_DIALOG] != NULL) {
    return;
  }

  buttontext = "";
  *tbuff = '\0';
  /* CONTEXT: export playlist dialog: export button */
  buttontext = _("Export");
  /* CONTEXT: export for ballroomdj: title of dialog */
  snprintf (tbuff, sizeof (tbuff), _("Export for %s"), BDJ4_NAME);

  szgrp = uiCreateSizeGroupHoriz ();

  uiexppl->wcont [UIEXPPL_W_DIALOG] =
      uiCreateDialog (uiexppl->parentwin,
      uiexppl->callbacks [UIEXPPL_CB_DIALOG],
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
      uiexppl->wcont [UIEXPPL_W_DIALOG], vbox);

  /* status msg */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiexppl->wcont [UIEXPPL_W_STATUS_MSG] = uiwidgetp;

  /* error msg */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, ERROR_CLASS);
  uiexppl->wcont [UIEXPPL_W_ERROR_MSG] = uiwidgetp;

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
  uiexppl->wcont [UIEXPPL_W_TARGET] = uiwidgetp;

  odir = nlistGetStr (uiexppl->options, MANAGE_EXP_BDJ4_DIR);
  if (odir == NULL) {
    odir = sysvarsGetStr (SV_HOME);
  }
  strlcpy (tbuff, odir, sizeof (tbuff));
  pathDisplayPath (tbuff, sizeof (tbuff));
  uiEntrySetValue (uiexppl->wcont [UIEXPPL_W_TARGET], tbuff);
  uiEntrySetValidate (uiwidgetp,
      uiexpplValidateTarget, uiexppl, UIENTRY_DELAYED);

  uiwidgetp = uiCreateButton (
      uiexppl->callbacks [UIEXPPL_CB_TARGET],
      "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiexppl->wcont [UIEXPPL_W_TGT_BUTTON] = uiwidgetp;

  uiwcontFree (hbox);

  /* playlist selector */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: export/import bdj4: playlist: select the song list */
  uiwidgetp = uiCreateColonLabel (_("Song List"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiexppl->uiplaylist = uiplaylistCreate (
      uiexppl->wcont [UIEXPPL_W_DIALOG],
      hbox, PL_LIST_NORMAL);
  uiplaylistSetSelectCallback (uiexppl->uiplaylist,
      uiexppl->callbacks [UIEXPPL_CB_SEL]);

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
  uiexppl->wcont [UIEXPPL_W_NEWNAME] = uiwidgetp;

  uiEntrySetValidate (uiwidgetp,
      uiexpplValidateNewName, uiexppl, UIENTRY_IMMEDIATE);

  uiwcontFree (hbox);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  logProcEnd (LOG_PROC, "uiexpplCreateDialog", "");
}

static bool
uiexpplTargetDialog (void *udata)
{
  UIEXPPL_t  *uiexppl = udata;
  uiselect_t  *selectdata;
  const char  *odir = NULL;
  char        *dir = NULL;

  if (uiexppl == NULL) {
    return UICB_STOP;
  }

  odir = uiEntryGetValue (uiexppl->wcont [UIEXPPL_W_TARGET]);
  selectdata = uiSelectInit (uiexppl->parentwin,
      /* CONTEXT: export/import bdj4 folder selection dialog: window title */
      _("Select Folder"), odir, NULL, NULL, NULL);

  dir = uiSelectDirDialog (selectdata);
  if (dir != NULL) {
    /* the validation process will be called */
    uiEntrySetValue (uiexppl->wcont [UIEXPPL_W_TARGET], dir);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", dir);
    mdfree (dir);   // allocated by gtk
  }
  uiSelectFree (selectdata);

  return UICB_CONT;
}


static void
uiexpplInitDisplay (UIEXPPL_t *uiexppl)
{
  if (uiexppl == NULL) {
    return;
  }
}

static bool
uiexpplResponseHandler (void *udata, long responseid)
{
  UIEXPPL_t  *uiexppl = udata;
  int             x, y, ws;

  uiWindowGetPosition (
      uiexppl->wcont [UIEXPPL_W_DIALOG], &x, &y, &ws);
  nlistSetNum (uiexppl->options, EXP_PL_POSITION_X, x);
  nlistSetNum (uiexppl->options, EXP_PL_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: del window");
      uiexpplFreeDialog (uiexppl);
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: close window");
      uiWidgetHide (uiexppl->wcont [UIEXPPL_W_DIALOG]);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: apply");
      uiLabelSetText (
          uiexppl->wcont [UIEXPPL_W_STATUS_MSG],
          /* CONTEXT: please wait... status message */
          _("Please wait\xe2\x80\xa6"));
      /* do not close or hide the dialog; it will stay active and */
      /* the status message will be updated */
      if (uiexppl->responsecb != NULL) {
        callbackHandler (uiexppl->responsecb);
      }
      break;
    }
  }

  uiexppl->isactive = false;
  return UICB_CONT;
}

static void
uiexpplFreeDialog (UIEXPPL_t *uiexppl)
{
  for (int j = 0; j < UIEXPPL_W_MAX; ++j) {
    uiwcontFree (uiexppl->wcont [j]);
    uiexppl->wcont [j] = NULL;
  }
  uiwcontFree (uiexppl->wcont [UIEXPPL_W_TGT_BUTTON]);
  uiexppl->wcont [UIEXPPL_W_TGT_BUTTON] = NULL;
  uiplaylistFree (uiexppl->uiplaylist);
  uiexppl->uiplaylist = NULL;
}

static int
uiexpplValidateTarget (uiwcont_t *entry, void *udata)
{
  UIEXPPL_t  *uiexppl = udata;
  const char  *str;
  char        tbuff [MAXPATHLEN];

  uiLabelSetText (
      uiexppl->wcont [UIEXPPL_W_ERROR_MSG], "");

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
      uiLabelSetText (uiexppl->wcont [UIEXPPL_W_ERROR_MSG],
          /* CONTEXT: export/import bdj4: invalid target folder */
          _("Invalid Folder"));
    }
    return UIENTRY_ERROR;
  }

  return UIENTRY_OK;
}


static bool
uiexpplSelectHandler (void *udata, long idx)
{
  UIEXPPL_t  *uiexppl = udata;
  const char  *str;

  str = uiplaylistGetValue (uiexppl->uiplaylist);
  uiEntrySetValue (uiexppl->wcont [UIEXPPL_W_NEWNAME], str);
  return UICB_CONT;
}

static int
uiexpplValidateNewName (uiwcont_t *entry, void *udata)
{
  UIEXPPL_t  *uiexppl = udata;
  uiwcont_t   *statusMsg = NULL;
  uiwcont_t   *errorMsg = NULL;
  int         rc = UIENTRY_ERROR;
  const char  *str;
  char        fn [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];

  statusMsg = uiexppl->wcont [UIEXPPL_W_STATUS_MSG];
  errorMsg = uiexppl->wcont [UIEXPPL_W_ERROR_MSG];
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
