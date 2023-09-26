/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "bdj4intl.h"
#include "bdjopt.h"
#include "istring.h"
#include "itunes.h"
#include "log.h"
#include "manageui.h"
#include "mdebug.h"
#include "nlist.h"
#include "osprocess.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "procutil.h"
#include "sysvars.h"
#include "ui.h"
#include "callback.h"

enum {
  MANAGE_DB_CHECK_NEW,
  MANAGE_DB_COMPACT,
  MANAGE_DB_REORGANIZE,
  MANAGE_DB_UPD_FROM_TAGS,
  MANAGE_DB_WRITE_TAGS,
  MANAGE_DB_UPD_FROM_ITUNES,
  MANAGE_DB_REBUILD,
};

enum {
  MDB_CB_TOPDIR_SEL,
  MDB_CB_DB_CHG,
  MDB_CB_START,
  MDB_CB_STOP,
  MDB_CB_MAX,
};

typedef struct managedb {
  uiwcont_t         *windowp;
  nlist_t           *options;
  uiwcont_t         *statusMsg;
  uiwcont_t         *errorMsg;
  const char        *pleasewaitmsg;
  procutil_t        **processes;
  conn_t            *conn;
  uientry_t         *dbtopdir;
  uibutton_t        *topdirsel;
  callback_t        *callbacks [MDB_CB_MAX];
  uispinbox_t       *dbspinbox;
  uibutton_t        *dbstart;
  uibutton_t        *dbstop;
  uiwcont_t         *dbhelpdisp;
  uitextbox_t       *dbstatus;
  nlist_t           *dblist;
  int               dblistWidth;
  nlist_t           *dbhelp;
  uiwcont_t         *dbpbar;
  bool              compact : 1;
} managedb_t;

static bool manageDbStart (void *udata);
static bool manageDbStop (void *udata);
static bool manageDbSelectDirCallback (void *udata);

managedb_t *
manageDbAlloc (uiwcont_t *window, nlist_t *options,
    uiwcont_t *statusMsg, uiwcont_t *errorMsg, const char *pleasewaitmsg,
    conn_t *conn, procutil_t **processes)
{
  managedb_t      *managedb;
  nlist_t         *tlist;
  nlist_t         *hlist;
  char            tbuff [300];
  int             maxw = 10;
  nlistidx_t      iteridx;
  const char      *str;

  managedb = mdmalloc (sizeof (managedb_t));

  managedb->windowp = window;
  managedb->conn = conn;
  managedb->processes = processes;
  managedb->dblist = NULL;
  managedb->dblistWidth = 0;
  managedb->dbhelp = NULL;
  managedb->dbpbar = NULL;
  managedb->topdirsel = NULL;
  managedb->dbstart = NULL;
  managedb->dbstop = NULL;
  managedb->dbhelpdisp = NULL;
  managedb->dbtopdir = uiEntryInit (50, 200);
  managedb->dbspinbox = uiSpinboxInit ();
  managedb->statusMsg = statusMsg;
  managedb->errorMsg = errorMsg;
  managedb->pleasewaitmsg = pleasewaitmsg;
  managedb->compact = false;
  for (int i = 0; i < MDB_CB_MAX; ++i) {
    managedb->callbacks [i] = NULL;
  }

  tlist = nlistAlloc ("db-action", LIST_ORDERED, NULL);
  hlist = nlistAlloc ("db-action-help", LIST_ORDERED, NULL);

  /* CONTEXT: database update: check for new audio files */
  nlistSetStr (tlist, MANAGE_DB_CHECK_NEW, _("Check For New"));
  nlistSetStr (hlist, MANAGE_DB_CHECK_NEW,
      /* CONTEXT: database update: check for new: help text */
      _("Checks for new audio files."));

  /* CONTEXT: database update: check for new audio files */
  nlistSetStr (tlist, MANAGE_DB_COMPACT, _("Compact"));
  nlistSetStr (hlist, MANAGE_DB_COMPACT,
      /* CONTEXT: database update: compact: help text */
      _("Compact the database.  Use after audio files have been deleted."));

  /* CONTEXT: database update: reorganize : renames audio files based on organization settings */
  nlistSetStr (tlist, MANAGE_DB_REORGANIZE, _("Reorganise"));
  nlistSetStr (hlist, MANAGE_DB_REORGANIZE,
      /* CONTEXT: database update: reorganize : help text */
      _("Renames the audio files based on the organization settings."));

  /* CONTEXT: database update: updates the database using the tags from the audio files */
  nlistSetStr (tlist, MANAGE_DB_UPD_FROM_TAGS, _("Update from Audio File Tags"));
  nlistSetStr (hlist, MANAGE_DB_UPD_FROM_TAGS,
      /* CONTEXT: database update: update from audio file tags: help text */
      _("Updates the information in the BallroomDJ database from the audio file tags."));

  /* CONTEXT: database update: writes the tags in the database to the audio files */
  nlistSetStr (tlist, MANAGE_DB_WRITE_TAGS, _("Write Tags to Audio Files"));
  nlistSetStr (hlist, MANAGE_DB_WRITE_TAGS,
      /* CONTEXT: database update: write tags to audio files: help text */
      _("Writes the audio file tags using the information from the BallroomDJ database."));

  /* CONTEXT: database update: update from itunes */
  snprintf (tbuff, sizeof (tbuff), _("Update from %s"), ITUNES_NAME);
  nlistSetStr (tlist, MANAGE_DB_UPD_FROM_ITUNES, tbuff);
  snprintf (tbuff, sizeof (tbuff),
      /* CONTEXT: database update: update from itunes: help text */
      _("Updates the information in the BallroomDJ database from the %s database."),
      ITUNES_NAME);
  nlistSetStr (hlist, MANAGE_DB_UPD_FROM_ITUNES, tbuff);

  /* CONTEXT: database update: rebuilds the database */
  nlistSetStr (tlist, MANAGE_DB_REBUILD, _("Rebuild Database"));
  nlistSetStr (hlist, MANAGE_DB_REBUILD,
      /* CONTEXT: database update: rebuild: help text */
      _("Replaces the BallroomDJ database in its entirety. All changes to the database will be lost."));
  managedb->dblist = tlist;
  managedb->dbhelp = hlist;

  nlistStartIterator (tlist, &iteridx);
  while ((str = nlistIterateValueData (tlist, &iteridx)) != NULL) {
    int     len;

    len = istrlen (str);
    if (len > maxw) {
      maxw = len;
    }
  }
  managedb->dblistWidth = maxw;

  return managedb;
}

void
manageDbFree (managedb_t *managedb)
{
  if (managedb != NULL) {
    procutilStopAllProcess (managedb->processes, managedb->conn, PROCUTIL_NORM_TERM);
    nlistFree (managedb->dblist);
    nlistFree (managedb->dbhelp);

    uiwcontFree (managedb->dbpbar);
    uiTextBoxFree (managedb->dbstatus);
    uiEntryFree (managedb->dbtopdir);
    uiSpinboxFree (managedb->dbspinbox);
    uiButtonFree (managedb->topdirsel);
    uiButtonFree (managedb->dbstart);
    uiButtonFree (managedb->dbstop);
    for (int i = 0; i < MDB_CB_MAX; ++i) {
      callbackFree (managedb->callbacks [i]);
    }
    uiwcontFree (managedb->dbhelpdisp);
    mdfree (managedb);
  }
}

void
manageDbProcess (managedb_t *managedb)
{
  if (managedb == NULL) {
    return;
  }

  uiEntryValidate (managedb->dbtopdir, false);
}

void
manageBuildUIUpdateDatabase (managedb_t *managedb, uiwcont_t *vboxp)
{
  uiwcont_t     *uiwidgetp;
  uiwcont_t     *hbox;
  uiwcont_t     *szgrp;
  uitextbox_t   *tb;
  char          tbuff [MAXPATHLEN];


  szgrp = uiCreateSizeGroupHoriz ();   // labels

  /* action selection */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxp, hbox);

  /* CONTEXT: update database: select database update action */
  uiwidgetp = uiCreateColonLabel (_("Action"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 2);
  uiwcontFree (uiwidgetp);

  uiSpinboxTextCreate (managedb->dbspinbox, managedb);
  uiSpinboxTextSet (managedb->dbspinbox, 0,
      nlistGetCount (managedb->dblist), managedb->dblistWidth,
      managedb->dblist, NULL, NULL);
  uiSpinboxTextSetValue (managedb->dbspinbox, MANAGE_DB_CHECK_NEW);
  uiwidgetp = uiSpinboxGetWidgetContainer (managedb->dbspinbox);
  managedb->callbacks [MDB_CB_DB_CHG] = callbackInit (
      manageDbChg, managedb, NULL);
  uiSpinboxTextSetValueChangedCallback (managedb->dbspinbox,
      managedb->callbacks [MDB_CB_DB_CHG]);
  uiBoxPackStart (hbox, uiwidgetp);

  /* help display */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vboxp, hbox);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 2);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("");
  uiLabelWrapOn (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 6);
  managedb->dbhelpdisp = uiwidgetp;

  /* db top dir  */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vboxp, hbox);

  /* CONTEXT: update database: music folder to process */
  uiwidgetp = uiCreateColonLabel (_("Music Folder"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 2);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiEntryCreate (managedb->dbtopdir);
  strlcpy (tbuff, bdjoptGetStr (OPT_M_DIR_MUSIC), sizeof (tbuff));
  pathDisplayPath (tbuff, sizeof (tbuff));
  uiEntrySetValue (managedb->dbtopdir, tbuff);
  uiwidgetp = uiEntryGetWidgetContainer (managedb->dbtopdir);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiEntrySetValidate (managedb->dbtopdir,
      uiEntryValidateDir, NULL, UIENTRY_DELAYED);

  managedb->callbacks [MDB_CB_TOPDIR_SEL] = callbackInit (
      manageDbSelectDirCallback, managedb, NULL);
  managedb->topdirsel = uiCreateButton (
      managedb->callbacks [MDB_CB_TOPDIR_SEL], "", NULL);
  uiButtonSetImageIcon (managedb->topdirsel, "folder");
  uiwidgetp = uiButtonGetWidgetContainer (managedb->topdirsel);
  uiBoxPackStart (hbox, uiwidgetp);

  /* buttons */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxp, hbox);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  managedb->callbacks [MDB_CB_START] = callbackInit (
      manageDbStart, managedb, NULL);
  managedb->dbstart = uiCreateButton (managedb->callbacks [MDB_CB_START],
      /* CONTEXT: update database: button to start the database update process */
      _("Start"), NULL);
  uiwidgetp = uiButtonGetWidgetContainer (managedb->dbstart);
  uiBoxPackStart (hbox, uiwidgetp);

  managedb->callbacks [MDB_CB_STOP] = callbackInit (
      manageDbStop, managedb, NULL);
  managedb->dbstop = uiCreateButton (managedb->callbacks [MDB_CB_STOP],
      /* CONTEXT: update database: button to stop the database update process */
      _("Stop"), NULL);
  uiwidgetp = uiButtonGetWidgetContainer (managedb->dbstop);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetState (uiwidgetp, UIWIDGET_DISABLE);

  managedb->dbpbar = uiCreateProgressBar ();
  uiWidgetSetClass (managedb->dbpbar, ACCENT_CLASS);
  uiWidgetSetMarginStart (managedb->dbpbar, 2);
  uiWidgetSetMarginEnd (managedb->dbpbar, 2);
  uiBoxPackStart (vboxp, managedb->dbpbar);

  tb = uiTextBoxCreate (200, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiTextBoxSetReadonly (tb);
  uiTextBoxDarken (tb);
  uiTextBoxSetHeight (tb, 300);
  uiBoxPackStartExpand (vboxp, uiTextBoxGetScrolledWindow (tb));
  managedb->dbstatus = tb;

  uiwcontFree (hbox);
  uiwcontFree (szgrp);
}

bool
manageDbChg (void *udata)
{
  managedb_t      *managedb = udata;
  double          value;
  int             nval;
  const char      *sval;

  nval = MANAGE_DB_CHECK_NEW;

  value = uiSpinboxTextGetValue (managedb->dbspinbox);
  nval = (int) value;

  uiLabelSetText (managedb->errorMsg, "");
  sval = nlistGetStr (managedb->dbhelp, nval);
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: db chg selector : %s", sval);

  if (managedb->dbhelpdisp != NULL) {
    uiwcont_t   *uiwidgetp;

    uiLabelSetText (managedb->dbhelpdisp, sval);
    uiWidgetRemoveClass (managedb->dbhelpdisp, ACCENT_CLASS);
    if (nval == MANAGE_DB_REBUILD) {
      uiWidgetSetClass (managedb->dbhelpdisp, ACCENT_CLASS);
    }
    uiwidgetp = uiButtonGetWidgetContainer (managedb->dbstart);
    uiWidgetSetState (uiwidgetp, UIWIDGET_ENABLE);
    if (nval == MANAGE_DB_UPD_FROM_ITUNES) {
      if (! itunesConfigured ()) {
        char  tbuff [200];

        /* CONTEXT: manage ui: status message: itunes is not configured */
        snprintf (tbuff, sizeof (tbuff), _("%s is not configured."), ITUNES_NAME);
        uiLabelSetText (managedb->errorMsg, tbuff);
        uiwidgetp = uiButtonGetWidgetContainer (managedb->dbstart);
        uiWidgetSetState (uiwidgetp, UIWIDGET_DISABLE);
      }
    }
  }
  return UICB_CONT;
}

void
manageDbProgressMsg (managedb_t *managedb, char *args)
{
  double    progval;

  if (strncmp ("END", args, 3) == 0) {
    uiProgressBarSet (managedb->dbpbar, 100.0);
  } else {
    if (sscanf (args, "PROG %lf", &progval) == 1) {
      uiProgressBarSet (managedb->dbpbar, progval);
    }
  }
}

void
manageDbStatusMsg (managedb_t *managedb, char *args)
{
  uiTextBoxAppendStr (managedb->dbstatus, args);
  uiTextBoxAppendStr (managedb->dbstatus, "\n");
  uiTextBoxScrollToEnd (managedb->dbstatus);
}

void
manageDbFinish (managedb_t *managedb, int routefrom)
{
  procutilCloseProcess (managedb->processes [routefrom],
      managedb->conn, ROUTE_DBUPDATE);
  procutilFreeRoute (managedb->processes, routefrom);
  connDisconnect (managedb->conn, routefrom);
}

void
manageDbClose (managedb_t *managedb)
{
  procutilStopAllProcess (managedb->processes, managedb->conn, PROCUTIL_FORCE_TERM);
  procutilFreeAll (managedb->processes);
}

void
manageDbResetButtons (managedb_t *managedb)
{
  if (managedb->compact) {
    char  tbuff [200];

    /* CONTEXT: update database: exit BDJ4 and restart */
    snprintf (tbuff, sizeof (tbuff), _("Exit %s and restart."), BDJ4_NAME);
    uiLabelSetText (managedb->statusMsg, tbuff);
  } else {
    uiLabelSetText (managedb->statusMsg, "");
  }
  managedb->compact = false;

  uiButtonSetState (managedb->dbstart, UIWIDGET_ENABLE);
  uiButtonSetState (managedb->dbstop, UIWIDGET_DISABLE);
  uiSpinboxSetState (managedb->dbspinbox, UIWIDGET_ENABLE);
}

/* internal routines */

static bool
manageDbStart (void *udata)
{
  managedb_t  *managedb = udata;
  int         nval;
  const char  *sval = NULL;
  const char  *targv [10];
  int         targc = 0;
  char        tbuff [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: db start");

  uiLabelSetText (managedb->statusMsg, "");
  uiLabelSetText (managedb->errorMsg, "");

  uiButtonSetState (managedb->dbstart, UIWIDGET_DISABLE);
  uiButtonSetState (managedb->dbstop, UIWIDGET_ENABLE);
  uiSpinboxSetState (managedb->dbspinbox, UIWIDGET_DISABLE);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "bdj4dbupdate", sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_DIR_EXEC);

  nval = uiSpinboxTextGetValue (managedb->dbspinbox);

  sval = nlistGetStr (managedb->dblist, nval);
  uiTextBoxAppendStr (managedb->dbstatus, "-- ");
  uiTextBoxAppendStr (managedb->dbstatus, sval);
  uiTextBoxAppendStr (managedb->dbstatus, "\n");

  uiLabelSetText (managedb->statusMsg, managedb->pleasewaitmsg);

  switch (nval) {
    case MANAGE_DB_CHECK_NEW: {
      targv [targc++] = "--checknew";
      break;
    }
    case MANAGE_DB_COMPACT: {
      managedb->compact = true;
      targv [targc++] = "--compact";
      break;
    }
    case MANAGE_DB_REORGANIZE: {
      targv [targc++] = "--reorganize";
      break;
    }
    case MANAGE_DB_UPD_FROM_TAGS: {
      targv [targc++] = "--updfromtags";
      break;
    }
    case MANAGE_DB_UPD_FROM_ITUNES: {
      targv [targc++] = "--updfromitunes";
      break;
    }
    case MANAGE_DB_WRITE_TAGS: {
      targv [targc++] = "--writetags";
      break;
    }
    case MANAGE_DB_REBUILD: {
      targv [targc++] = "--rebuild";
      break;
    }
  }

  targv [targc++] = "--progress";
  targv [targc++] = "--dbtopdir";
  strlcpy (tbuff, uiEntryGetValue (managedb->dbtopdir), sizeof (tbuff));
  pathNormalizePath (tbuff, sizeof (tbuff));
  targv [targc++] = tbuff;
  targv [targc++] = NULL;
  logMsg (LOG_DBG, LOG_BASIC, "start dbupdate %s", uiEntryGetValue (managedb->dbtopdir));

  uiProgressBarSet (managedb->dbpbar, 0.0);
  managedb->processes [ROUTE_DBUPDATE] = procutilStartProcess (
      ROUTE_DBUPDATE, "bdj4dbupdate", OS_PROC_DETACH, targv);
  return UICB_CONT;
}

static bool
manageDbStop (void *udata)
{
  managedb_t  *managedb = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: db stop");
  connSendMessage (managedb->conn, ROUTE_DBUPDATE, MSG_DB_STOP_REQ, NULL);
  manageDbResetButtons (managedb);
  return UICB_CONT;
}


static bool
manageDbSelectDirCallback (void *udata)
{
  managedb_t  *managedb = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;
  char        tbuff [100];

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: db select top dir");
  /* CONTEXT: update database: dialog title for selecting database music folder */
  snprintf (tbuff, sizeof (tbuff), _("Select Music Folder Location"));
  selectdata = uiDialogCreateSelect (managedb->windowp,
      tbuff, uiEntryGetValue (managedb->dbtopdir), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (managedb->dbtopdir, fn);
    mdfree (fn);
  }
  mdfree (selectdata);
  return UICB_CONT;
}


