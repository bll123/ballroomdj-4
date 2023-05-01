/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
#include "callback.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "playlist.h"
#include "sysvars.h"
#include "ui.h"
#include "uiexpimpbdj4.h"
#include "uiplaylist.h"
#include "uiutils.h"
#include "validate.h"

enum {
  UIEIBDJ4_CB_DIALOG,
  UIEIBDJ4_CB_TARGET,
  UIEIBDJ4_CB_SEL,
  UIEIBDJ4_CB_MAX,
};

enum {
  UIEIBDJ4_W_STATUS_MSG,
  UIEIBDJ4_W_ERROR_MSG,
  UIEIBDJ4_W_DIALOG,
  UIEIBDJ4_W_MAX,
};

typedef struct {
  uiwcont_t       *wcont [UIEIBDJ4_W_MAX];
  uientry_t       *target;
  uientry_t       *newname;
  uiplaylist_t    *uiplaylist;
  uibutton_t      *targetButton;
  callback_t      *responsecb;
} uieibdj4dialog_t;

typedef struct uieibdj4 {
  uiwcont_t         *parentwin;
  nlist_t           *options;
  uieibdj4dialog_t  dialog [UIEIBDJ4_MAX];
  int               currtype;
  callback_t        *callbacks [UIEIBDJ4_CB_MAX];
  bool              isactive : 1;
} uieibdj4_t;

/* export/import bdj4 */
static void   uieibdj4CreateDialog (uieibdj4_t *uieibdj4);
static bool   uieibdj4TargetDialog (void *udata);
static void   uieibdj4InitDisplay (uieibdj4_t *uieibdj4);
static bool   uieibdj4ResponseHandler (void *udata, long responseid);
static void   uieibdj4FreeDialog (uieibdj4_t *uieibdj4, int expimptype);
static int    uieibdj4ValidateTarget (uientry_t *entry, void *udata);
static bool   uieibdj4SelectHandler (void *udata, long idx);
static int    uieibdj4ValidateNewName (uientry_t *entry, void *udata);

uieibdj4_t *
uieibdj4Init (uiwcont_t *windowp, nlist_t *opts)
{
  uieibdj4_t  *uieibdj4;

  uieibdj4 = mdmalloc (sizeof (uieibdj4_t));
  for (int i = 0; i < UIEIBDJ4_MAX; ++i) {
    for (int j = 0; j < UIEIBDJ4_W_MAX; ++j) {
      uieibdj4->dialog [i].wcont [j] = NULL;
    }
    uieibdj4->dialog [i].target = uiEntryInit (50, MAXPATHLEN);
    uieibdj4->dialog [i].newname = uiEntryInit (30, MAXPATHLEN);
    uieibdj4->dialog [i].targetButton = NULL;
    uieibdj4->dialog [i].uiplaylist = NULL;
    uieibdj4->dialog [i].responsecb = NULL;
  }
  uieibdj4->parentwin = windowp;
  uieibdj4->options = opts;
  for (int i = 0; i < UIEIBDJ4_CB_MAX; ++i) {
    uieibdj4->callbacks [i] = NULL;
  }
  uieibdj4->isactive = false;

  uieibdj4->callbacks [UIEIBDJ4_CB_DIALOG] = callbackInitLong (
      uieibdj4ResponseHandler, uieibdj4);
  uieibdj4->callbacks [UIEIBDJ4_CB_TARGET] = callbackInit (
      uieibdj4TargetDialog, uieibdj4, NULL);
  uieibdj4->callbacks [UIEIBDJ4_CB_SEL] = callbackInitLong (
      uieibdj4SelectHandler, uieibdj4);

  return uieibdj4;
}

void
uieibdj4Free (uieibdj4_t *uieibdj4)
{
  if (uieibdj4 != NULL) {
    for (int i = 0; i < UIEIBDJ4_CB_MAX; ++i) {
      callbackFree (uieibdj4->callbacks [i]);
    }
    for (int i = 0; i < UIEIBDJ4_MAX; ++i) {
      uieibdj4FreeDialog (uieibdj4, i);
    }
    mdfree (uieibdj4);
  }
}

void
uieibdj4SetResponseCallback (uieibdj4_t *uieibdj4,
    callback_t *uicb, int expimptype)
{
  if (uieibdj4 == NULL) {
    return;
  }
  if (expimptype < 0 || expimptype >= UIEIBDJ4_MAX) {
    return;
  }
  uieibdj4->dialog [expimptype].responsecb = uicb;
}

bool
uieibdj4Dialog (uieibdj4_t *uieibdj4, int expimptype)
{
  int         x, y;

  if (uieibdj4 == NULL) {
    return UICB_STOP;
  }
  if (expimptype < 0 || expimptype >= UIEIBDJ4_MAX) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uieibdj4Dialog");
  uieibdj4->currtype = expimptype;
  uieibdj4CreateDialog (uieibdj4);
  uieibdj4InitDisplay (uieibdj4);
  uiDialogShow (uieibdj4->dialog [expimptype].wcont [UIEIBDJ4_W_DIALOG]);
  uieibdj4->isactive = true;

  x = nlistGetNum (uieibdj4->options, EXP_IMP_BDJ4_POSITION_X);
  y = nlistGetNum (uieibdj4->options, EXP_IMP_BDJ4_POSITION_Y);
  uiWindowMove (uieibdj4->dialog [expimptype].wcont [UIEIBDJ4_W_DIALOG], x, y, -1);
  logProcEnd (LOG_PROC, "uieibdj4Dialog", "");
  return UICB_CONT;
}

void
uieibdj4DialogClear (uieibdj4_t *uieibdj4)
{
  if (uieibdj4 == NULL) {
    return;
  }

  uiWidgetHide (uieibdj4->dialog [uieibdj4->currtype].wcont [UIEIBDJ4_W_DIALOG]);
}

/* delayed entry validation for the audio file needs to be run */
void
uieibdj4Process (uieibdj4_t *uieibdj4)
{
  if (uieibdj4 == NULL) {
    return;
  }
  if (! uieibdj4->isactive) {
    return;
  }

  uiEntryValidate (uieibdj4->dialog [uieibdj4->currtype].target, false);
}

const char *
uieibdj4GetDir (uieibdj4_t *uieibdj4)
{
  const char  *dir;

  if (uieibdj4 == NULL) {
    return NULL;
  }

  dir = uiEntryGetValue (uieibdj4->dialog [uieibdj4->currtype].target);
  return dir;
}

const char *
uieibdj4GetPlaylist (uieibdj4_t *uieibdj4)
{
  const char  *plname;

  if (uieibdj4 == NULL) {
    return NULL;
  }

  plname = uiplaylistGetValue (uieibdj4->dialog [uieibdj4->currtype].uiplaylist);
  return plname;
}

const char *
uieibdj4GetNewName (uieibdj4_t *uieibdj4)
{
  const char  *newname;

  if (uieibdj4 == NULL) {
    return NULL;
  }

  newname = uiEntryGetValue (uieibdj4->dialog [uieibdj4->currtype].newname);
  return newname;
}

void
uieibdj4UpdateStatus (uieibdj4_t *uieibdj4, int count, int tot)
{
  if (uieibdj4 == NULL) {
    return;
  }

  uiutilsProgressStatus (
      uieibdj4->dialog [uieibdj4->currtype].wcont [UIEIBDJ4_W_STATUS_MSG],
      count, tot);
}

/* internal routines */

static void
uieibdj4CreateDialog (uieibdj4_t *uieibdj4)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp = NULL;
  uibutton_t    *uibutton;
  uiwcont_t     *szgrp;  // labels
  const char    *buttontext;
  char          tbuff [100];
  int           currtype;
  const char    *odir = NULL;

  logProcBegin (LOG_PROC, "uieibdj4CreateDialog");

  if (uieibdj4 == NULL) {
    return;
  }

  currtype = uieibdj4->currtype;
  buttontext = "";
  *tbuff = '\0';
  if (currtype == UIEIBDJ4_EXPORT) {
    /* CONTEXT: export/import bdj4 dialog: export button */
    buttontext = _("Export");
    /* CONTEXT: export for ballroomdj: title of dialog */
    snprintf (tbuff, sizeof (tbuff), _("Export for %s"), BDJ4_NAME);
  }
  if (currtype == UIEIBDJ4_IMPORT) {
    /* CONTEXT: export/import bdj4 dialog: import button */
    buttontext = _("Import");
    /* CONTEXT: import from ballroomdj: title of dialog */
    snprintf (tbuff, sizeof (tbuff), _("Import from %s"), BDJ4_NAME);
  }

  if (uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_DIALOG] != NULL) {
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();

  uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_DIALOG] =
      uiCreateDialog (uieibdj4->parentwin,
      uieibdj4->callbacks [UIEIBDJ4_CB_DIALOG],
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
      uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_DIALOG], vbox);

  /* status msg */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_STATUS_MSG] = uiwidgetp;

  /* error msg */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, ERROR_CLASS);
  uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_ERROR_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* target folder */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  if (currtype == UIEIBDJ4_EXPORT) {
    uiwidgetp = uiCreateColonLabel (
        /* CONTEXT: export/import bdj4: export folder location */
        _("Export to"));
  }
  if (currtype == UIEIBDJ4_IMPORT) {
    uiwidgetp = uiCreateColonLabel (
        /* CONTEXT: export/import bdj4: import folder location */
        _("Import from"));
  }
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiEntryCreate (uieibdj4->dialog [currtype].target);
  uiEntrySetValue (uieibdj4->dialog [currtype].target, "");
  uiwidgetp = uiEntryGetWidgetContainer (uieibdj4->dialog [currtype].target);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);

  if (uieibdj4->currtype == UIEIBDJ4_EXPORT) {
    odir = nlistGetStr (uieibdj4->options, MANAGE_EXP_BDJ4_DIR);
  }
  if (uieibdj4->currtype == UIEIBDJ4_IMPORT) {
    odir = nlistGetStr (uieibdj4->options, MANAGE_IMP_BDJ4_DIR);
  }
  if (odir == NULL) {
    odir = sysvarsGetStr (SV_HOME);
  }
  uiEntrySetValue (uieibdj4->dialog [uieibdj4->currtype].target, odir);

  uibutton = uiCreateButton (
      uieibdj4->callbacks [UIEIBDJ4_CB_TARGET],
      "", NULL);
  uieibdj4->dialog [currtype].targetButton = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);

  uiEntrySetValidate (uieibdj4->dialog [currtype].target,
      uieibdj4ValidateTarget, uieibdj4, UIENTRY_DELAYED);

  if (currtype == UIEIBDJ4_IMPORT) {
    uiwcontFree (hbox);

    /* playlist selector */
    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vbox, hbox);

    /* CONTEXT: export/import bdj4: playlist: select the song list */
    uiwidgetp = uiCreateColonLabel (_("Song List"));
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrp, uiwidgetp);
    uiwcontFree (uiwidgetp);

    uieibdj4->dialog [currtype].uiplaylist = uiplaylistCreate (
        uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_DIALOG],
        hbox, PL_LIST_NORMAL);
    uiplaylistSetSelectCallback (uieibdj4->dialog [currtype].uiplaylist,
        uieibdj4->callbacks [UIEIBDJ4_CB_SEL]);

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

    uiEntryCreate (uieibdj4->dialog [currtype].newname);
    uiEntrySetValue (uieibdj4->dialog [currtype].newname, "");
    uiwidgetp = uiEntryGetWidgetContainer (uieibdj4->dialog [currtype].newname);
    uiBoxPackStart (hbox, uiwidgetp);

    uiEntrySetValidate (uieibdj4->dialog [currtype].newname,
        uieibdj4ValidateNewName, uieibdj4, UIENTRY_IMMEDIATE);
  }

  uiwcontFree (hbox);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  logProcEnd (LOG_PROC, "uieibdj4CreateDialog", "");
}

static bool
uieibdj4TargetDialog (void *udata)
{
  uieibdj4_t  *uieibdj4 = udata;
  uiselect_t  *selectdata;
  const char  *odir = NULL;
  char        *dir = NULL;

  if (uieibdj4 == NULL) {
    return UICB_STOP;
  }

  odir = uiEntryGetValue (uieibdj4->dialog [uieibdj4->currtype].target);
  selectdata = uiDialogCreateSelect (uieibdj4->parentwin,
      /* CONTEXT: export/import bdj4 folder selection dialog: window title */
      _("Select Folder"), odir, NULL, NULL, NULL);

  dir = uiSelectDirDialog (selectdata);
  if (dir != NULL) {
    /* the validation process will be called */
    uiEntrySetValue (uieibdj4->dialog [uieibdj4->currtype].target, dir);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", dir);
    mdfree (dir);   // allocated by gtk
  }
  mdfree (selectdata);

  return UICB_CONT;
}


static void
uieibdj4InitDisplay (uieibdj4_t *uieibdj4)
{
  if (uieibdj4 == NULL) {
    return;
  }
}

static bool
uieibdj4ResponseHandler (void *udata, long responseid)
{
  uieibdj4_t  *uieibdj4 = udata;
  int             x, y, ws;
  int             currtype;

  currtype = uieibdj4->currtype;
  uiWindowGetPosition (
      uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_DIALOG], &x, &y, &ws);
  nlistSetNum (uieibdj4->options, EXP_IMP_BDJ4_POSITION_X, x);
  nlistSetNum (uieibdj4->options, EXP_IMP_BDJ4_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: del window");
      uieibdj4FreeDialog (uieibdj4, currtype);
      uieibdj4->dialog [currtype].target = uiEntryInit (50, MAXPATHLEN);
      uieibdj4->dialog [currtype].newname = uiEntryInit (30, MAXPATHLEN);
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: close window");
      uiWidgetHide (uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_DIALOG]);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: apply");
      uiLabelSetText (
          uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_STATUS_MSG],
          /* CONTEXT: please wait... status message */
          _("Please wait\xe2\x80\xa6"));
      /* do not close or hide the dialog; it will stay active and */
      /* the status message will be updated */
      if (uieibdj4->dialog [currtype].responsecb != NULL) {
        callbackHandler (uieibdj4->dialog [currtype].responsecb);
      }
      break;
    }
  }

  uieibdj4->isactive = false;
  return UICB_CONT;
}

static void
uieibdj4FreeDialog (uieibdj4_t *uieibdj4, int expimptype)
{
  for (int j = 0; j < UIEIBDJ4_W_MAX; ++j) {
    uiwcontFree (uieibdj4->dialog [expimptype].wcont [j]);
    uieibdj4->dialog [expimptype].wcont [j] = NULL;
  }
  uiEntryFree (uieibdj4->dialog [expimptype].target);
  uieibdj4->dialog [expimptype].target = NULL;
  uiEntryFree (uieibdj4->dialog [expimptype].newname);
  uieibdj4->dialog [expimptype].newname = NULL;
  uiButtonFree (uieibdj4->dialog [expimptype].targetButton);
  uieibdj4->dialog [expimptype].targetButton = NULL;
  uiplaylistFree (uieibdj4->dialog [expimptype].uiplaylist);
  uieibdj4->dialog [expimptype].uiplaylist = NULL;
}

static int
uieibdj4ValidateTarget (uientry_t *entry, void *udata)
{
  uieibdj4_t  *uieibdj4 = udata;
  const char  *str;
  int         currtype;
  char        tbuff [MAXPATHLEN];

  currtype = uieibdj4->currtype;
  if (uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_ERROR_MSG] != NULL) {
    uiLabelSetText (
        uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_ERROR_MSG], "");
  }

  str = uiEntryGetValue (entry);
  *tbuff = '\0';
  if (str != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%s/data", str);
  }

  /* validation failures:
   *   target is not set (no message displayed)
   *   target is not a directory
   *   if importing, target/data is not a directory
   */
  if (! *str || ! fileopIsDirectory (str) ||
      (currtype == UIEIBDJ4_IMPORT && ! fileopIsDirectory (tbuff))) {
    if (*str) {
      uiLabelSetText (uieibdj4->dialog [currtype].wcont [UIEIBDJ4_W_ERROR_MSG],
          /* CONTEXT: export/import bdj4: invalid target folder */
          _("Invalid Folder"));
    }
    return UIENTRY_ERROR;
  }

  if (uieibdj4->currtype == UIEIBDJ4_IMPORT) {
    uiplaylistSetList (uieibdj4->dialog [currtype].uiplaylist,
        PL_LIST_DIR, tbuff);
  }
  return UIENTRY_OK;
}


static bool
uieibdj4SelectHandler (void *udata, long idx)
{
  uieibdj4_t  *uieibdj4 = udata;
  int         currtype;
  const char  *str;

  currtype = uieibdj4->currtype;
  str = uiplaylistGetValue (uieibdj4->dialog [currtype].uiplaylist);
  uiEntrySetValue (uieibdj4->dialog [currtype].newname, str);
  return UICB_CONT;
}

static int
uieibdj4ValidateNewName (uientry_t *entry, void *udata)
{
  uieibdj4_t  *uieibdj4 = udata;
  uiwcont_t   *statusMsg = NULL;
  uiwcont_t   *errorMsg = NULL;
  int         rc = UIENTRY_ERROR;
  const char  *str;
  char        fn [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];

  statusMsg = uieibdj4->dialog [uieibdj4->currtype].wcont [UIEIBDJ4_W_STATUS_MSG];
  errorMsg = uieibdj4->dialog [uieibdj4->currtype].wcont [UIEIBDJ4_W_ERROR_MSG];
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
