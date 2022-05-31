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

#include "bdj4intl.h"
#include "bdjopt.h"
#include "manageui.h"
#include "nlist.h"
#include "osutils.h"
#include "pathbld.h"
#include "procutil.h"
#include "sysvars.h"
#include "ui.h"
#include "uiutils.h"

enum {
  MANAGE_DB_CHECK_NEW,
  MANAGE_DB_REORGANIZE,
  MANAGE_DB_UPD_FROM_TAGS,
  MANAGE_DB_WRITE_TAGS,
  MANAGE_DB_REBUILD,
};

typedef struct managedb {
  UIWidget          *windowp;
  nlist_t           *options;
  UIWidget          *statusMsg;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  uispinbox_t       *dbspinbox;
  UICallback        dbchgcb;
  UICallback        dbstartcb;
  uitextbox_t       *dbhelpdisp;
  uitextbox_t       *dbstatus;
  nlist_t           *dblist;
  nlist_t           *dbhelp;
  UIWidget          dbpbar;
} managedb_t;

static bool     manageDbStart (void *udata);

managedb_t *
manageDbAlloc (UIWidget *window, nlist_t *options,
    UIWidget *statusMsg, conn_t *conn)
{
  managedb_t      *managedb;
  nlist_t         *tlist;
  nlist_t         *hlist;

  managedb = malloc (sizeof (managedb_t));

  procutilInitProcesses (managedb->processes);
  managedb->conn = conn;
  managedb->dblist = NULL;
  managedb->dbhelp = NULL;
  uiutilsUIWidgetInit (&managedb->dbpbar);
  managedb->dbspinbox = uiSpinboxTextInit ();

  tlist = nlistAlloc ("db-action", LIST_ORDERED, free);
  hlist = nlistAlloc ("db-action-help", LIST_ORDERED, free);
  /* CONTEXT: database update: check for new audio files */
  nlistSetStr (tlist, MANAGE_DB_CHECK_NEW, _("Check For New"));
  nlistSetStr (hlist, MANAGE_DB_CHECK_NEW,
      /* CONTEXT: database update: check for new: help text */
      _("Checks for new audio files."));
  /* CONTEXT: database update: reorganize : renames audio files based on organization settings */
  nlistSetStr (tlist, MANAGE_DB_REORGANIZE, _("Reorganize"));
  nlistSetStr (hlist, MANAGE_DB_REORGANIZE,
      /* CONTEXT: database update: reorganize : help text */
      _("Renames the audio files based on the organization settings."));
  /* CONTEXT: database update: updates the database using the tags from the audio files */
  nlistSetStr (tlist, MANAGE_DB_UPD_FROM_TAGS, _("Update from Audio File Tags"));
  nlistSetStr (hlist, MANAGE_DB_UPD_FROM_TAGS,
      /* CONTEXT: database update: update from audio file tags: help text */
      _("Replaces the information in the BallroomDJ database with the audio file tag information."));
  /* CONTEXT: database update: writes the tags in the database to the audio files */
  nlistSetStr (tlist, MANAGE_DB_WRITE_TAGS, _("Write Tags to Audio Files"));
  nlistSetStr (hlist, MANAGE_DB_WRITE_TAGS,
      /* CONTEXT: database update: write tags to audio files: help text */
      _("Updates the audio file tags with the information from the BallroomDJ database."));
  /* CONTEXT: database update: rebuilds the database */
  nlistSetStr (tlist, MANAGE_DB_REBUILD, _("Rebuild Database"));
  nlistSetStr (hlist, MANAGE_DB_REBUILD,
      /* CONTEXT: database update: rebuild: help text */
      _("Replaces the BallroomDJ database in its entirety. All changes to the database will be lost."));
  managedb->dblist = tlist;
  managedb->dbhelp = hlist;

  return managedb;
}

void
manageDbFree (managedb_t *managedb)
{
  if (managedb != NULL) {
    procutilStopAllProcess (managedb->processes, managedb->conn, false);
    if (managedb->dblist != NULL) {
      nlistFree (managedb->dblist);
    }
    if (managedb->dbhelp != NULL) {
      nlistFree (managedb->dbhelp);
    }

    uiTextBoxFree (managedb->dbhelpdisp);
    uiTextBoxFree (managedb->dbstatus);
    uiSpinboxTextFree (managedb->dbspinbox);
    free (managedb);
  }
}


void
manageBuildUIUpdateDatabase (managedb_t *managedb, UIWidget *vboxp)
{
  UIWidget       uiwidget;
  UIWidget       *uiwidgetp;
  UIWidget       hbox;
  uitextbox_t    *tb;

  /* help display */
  tb = uiTextBoxCreate (80);
  uiTextBoxSetReadonly (tb);
  uiTextBoxSetHeight (tb, 70);
  uiBoxPackStart (vboxp, uiTextBoxGetScrolledWindow (tb));
  managedb->dbhelpdisp = tb;

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (vboxp, &hbox);

  uiSpinboxTextCreate (managedb->dbspinbox, managedb);
  /* currently hard-coded at 30 chars */
  uiSpinboxTextSet (managedb->dbspinbox, 0,
      nlistGetCount (managedb->dblist), 30,
      managedb->dblist, NULL, NULL);
  uiSpinboxTextSetValue (managedb->dbspinbox, MANAGE_DB_CHECK_NEW);
  uiwidgetp = uiSpinboxGetUIWidget (managedb->dbspinbox);
  uiutilsUICallbackInit (&managedb->dbchgcb, manageDbChg, managedb);
  uiSpinboxTextSetValueChangedCallback (managedb->dbspinbox, &managedb->dbchgcb);
//  g_signal_connect (uiwidgetp->widget, "value-changed", G_CALLBACK (manageDbChg), managedb);
  uiBoxPackStart (&hbox, uiwidgetp);


  uiutilsUICallbackInit (&managedb->dbstartcb, manageDbStart, managedb);
  uiCreateButton (&uiwidget, &managedb->dbstartcb,
      /* CONTEXT: update database: button to start the database update process */
      _("Start"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateProgressBar (&managedb->dbpbar, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiBoxPackStart (vboxp, &managedb->dbpbar);

  tb = uiTextBoxCreate (200);
  uiTextBoxSetReadonly (tb);
  uiTextBoxDarken (tb);
  uiTextBoxSetHeight (tb, 300);
  uiBoxPackStartExpand (vboxp, uiTextBoxGetScrolledWindow (tb));
  managedb->dbstatus = tb;
}

bool
manageDbChg (void *udata)
{
  managedb_t      *managedb = udata;
  double          value;
  ssize_t         nval;
  char            *sval;

  nval = MANAGE_DB_CHECK_NEW;

  value = uiSpinboxTextGetValue (managedb->dbspinbox);
  nval = (ssize_t) value;

  sval = nlistGetStr (managedb->dbhelp, nval);
  uiTextBoxSetValue (managedb->dbhelpdisp, sval);
  return UICB_CONT;
}

void
manageDbProgressMsg (managedb_t *managedb, char *args)
{
  double    progval;

  if (strncmp ("END", args, 3) == 0) {
    uiProgressBarSet (&managedb->dbpbar, 100.0);
  } else {
    if (sscanf (args, "PROG %lf", &progval) == 1) {
      uiProgressBarSet (&managedb->dbpbar, progval);
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
  procutilStopAllProcess (managedb->processes, managedb->conn, true);
  procutilFreeAll (managedb->processes);
}

/* internal routines */

static bool
manageDbStart (void *udata)
{
  managedb_t  *managedb = udata;
  int         nval;
  char        *sval = NULL;
  char        *targv [10];
  int         targc = 0;
  char        tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff),
      "bdj4dbupdate", sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_EXECDIR);

  nval = uiSpinboxTextGetValue (managedb->dbspinbox);

  sval = nlistGetStr (managedb->dblist, nval);
  uiTextBoxAppendStr (managedb->dbstatus, "-- ");
  uiTextBoxAppendStr (managedb->dbstatus, sval);
  uiTextBoxAppendStr (managedb->dbstatus, "\n");

  switch (nval) {
    case MANAGE_DB_CHECK_NEW: {
      targv [targc++] = "--checknew";
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
  targv [targc++] = NULL;

  uiProgressBarSet (&managedb->dbpbar, 0.0);
  managedb->processes [ROUTE_DBUPDATE] = procutilStartProcess (
      ROUTE_DBUPDATE, "bdj4dbupdate", OS_PROC_DETACH, targv);
  return UICB_CONT;
}

