/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "datafile.h"
#include "dirlist.h"
#include "dirop.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "instutil.h"
#include "localeutil.h"
#include "log.h"
#include "osprocess.h"
#include "osutils.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"

/* setup states */
typedef enum {
  ALT_PRE_INIT,
  ALT_PREPARE,
  ALT_WAIT_USER,
  ALT_INIT,
  ALT_MAKE_TARGET,
  ALT_CHDIR,
  ALT_CREATE_DIRS,
  ALT_COPY_TEMPLATES,
  ALT_SETUP,
  ALT_CREATE_SHORTCUT,
  ALT_UPDATE_PROCESS_INIT,
  ALT_UPDATE_PROCESS,
  ALT_FINISH,
  ALT_EXIT,
} altsetupstate_t;

enum {
  ALT_CB_TARGET_DIR,
  ALT_CB_EXIT,
  ALT_CB_START,
  ALT_CB_MAX,
};

enum {
  ALT_BUTTON_TARGET_DIR,
  ALT_BUTTON_EXIT,
  ALT_BUTTON_START,
  ALT_BUTTON_MAX,
};

enum {
  ALT_TARGET,
};

typedef struct {
  altsetupstate_t instState;
  UICallback      callbacks [ALT_CB_MAX];
  uibutton_t      *buttons [ALT_BUTTON_MAX];
  char            *target;
  char            *maindir;
  char            *hostname;
  char            *home;
  char            dlfname [MAXPATHLEN];
  /* conversion */
  UIWidget        window;
  uientry_t       *targetEntry;
  uientry_t       *nameEntry;
  UIWidget        reinstWidget;
  UICallback      reinstcb;
  UIWidget        feedbackMsg;
  uitextbox_t     *disptb;
  /* flags */
  bool            uiBuilt : 1;
  bool            scrolltoend : 1;
  bool            newinstall : 1;
  bool            reinstall : 1;
} altsetup_t;

static void altsetupBuildUI (altsetup_t *altsetup);
static int  altsetupMainLoop (void *udata);
static bool altsetupExitCallback (void *udata);
static bool altsetupCheckDirTarget (void *udata);
static bool altsetupTargetDirDialog (void *udata);
static int  altsetupValidateTarget (uientry_t *entry, void *udata);
static bool altsetupSetupCallback (void *udata);
static void altsetupSetPaths (altsetup_t *altsetup);

static void altsetupInit (altsetup_t *altsetup);
static void altsetupMakeTarget (altsetup_t *altsetup);
static void altsetupChangeDir (altsetup_t *altsetup);
static void altsetupCreateDirs (altsetup_t *altsetup);
static void altsetupCopyTemplates (altsetup_t *altsetup);
static void altsetupSetup (altsetup_t *altsetup);
static void altsetupCreateShortcut (altsetup_t *altsetup);
static void altsetupUpdateProcessInit (altsetup_t *altsetup);
static void altsetupUpdateProcess (altsetup_t *altsetup);

static void altsetupCleanup (altsetup_t *altsetup);
static void altsetupDisplayText (altsetup_t *altsetup, char *pfx, char *txt, bool bold);
static void altsetupTemplateCopy (const char *dir, const char *from, const char *to);
static void altsetupFailWorkingDir (altsetup_t *altsetup, const char *dir);
static void altsetupSetTargetDir (altsetup_t *altsetup, const char *fn);

int
main (int argc, char *argv[])
{
  altsetup_t   altsetup;
  char          buff [MAXPATHLEN];
  char          *uifont;

  buff [0] = '\0';

  uiutilsUIWidgetInit (&altsetup.window);
  altsetup.instState = ALT_PRE_INIT;
  altsetup.target = strdup ("");
  altsetup.uiBuilt = false;
  altsetup.scrolltoend = false;
  altsetup.maindir = NULL;
  altsetup.home = NULL;
  altsetup.hostname = NULL;
  altsetup.newinstall = false;
  altsetup.reinstall = false;
  uiutilsUIWidgetInit (&altsetup.reinstWidget);
  uiutilsUIWidgetInit (&altsetup.feedbackMsg);
  for (int i = 0; i < ALT_BUTTON_MAX; ++i) {
    altsetup.buttons [i] = NULL;
  }

  altsetup.targetEntry = uiEntryInit (80, MAXPATHLEN);
  altsetup.nameEntry = uiEntryInit (30, 30);

  sysvarsInit (argv[0]);
  localeInit ();
  bdjoptInit ();
  bdjvarsInit ();

  altsetup.maindir = sysvarsGetStr (SV_BDJ4_DIR_MAIN);
  altsetup.home = sysvarsGetStr (SV_HOME);
  altsetup.hostname = sysvarsGetStr (SV_HOSTNAME);

  uiUIInitialize ();

  uifont = sysvarsGetStr (SV_FONT_DEFAULT);
  if (uifont == NULL || ! *uifont) {
    uifont = "Arial Regular 14";
    if (isMacOS ()) {
      uifont = "Arial Regular 17";
    }
  }
  uiSetUIFont (uifont);

  altsetupBuildUI (&altsetup);
  osuiFinalize ();

  while (altsetup.instState != ALT_EXIT) {
    altsetupMainLoop (&altsetup);
    mssleep (10);
  }

  /* process any final events */
  uiUIProcessEvents ();

  altsetupCleanup (&altsetup);
  bdjvarsCleanup ();
  bdjoptCleanup ();
  localeCleanup ();
  logEnd ();
  return 0;
}

static void
altsetupBuildUI (altsetup_t *altsetup)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  uibutton_t    *uibutton;
  UIWidget      *uiwidgetp;
  char          tbuff [100];
  char          imgbuff [MAXPATHLEN];


  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&uiwidget);

  strlcpy (imgbuff, "img/bdj4_icon_inst.png", sizeof (imgbuff));
  osuiSetIcon (imgbuff);

  strlcpy (imgbuff, "img/bdj4_icon_inst.svg", sizeof (imgbuff));
  /* CONTEXT: set up alternate: window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Set Up Alternate"), BDJ4_NAME);
  uiutilsUICallbackInit (&altsetup->callbacks [ALT_CB_EXIT],
      altsetupExitCallback, altsetup, NULL);
  uiCreateMainWindow (&altsetup->window,
      &altsetup->callbacks [ALT_CB_EXIT],
      tbuff, imgbuff);
  uiWindowSetDefaultSize (&altsetup->window, 1000, 600);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 10);
  uiWidgetSetMarginTop (&vbox, 20);
  uiWidgetExpandHoriz (&vbox);
  uiWidgetExpandVert (&vbox);
  uiBoxPackInWindow (&altsetup->window, &vbox);

  uiCreateLabel (&uiwidget,
      /* CONTEXT: set up alternate: ask for alternate folder */
      _("Enter the alternate folder."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiEntryCreate (altsetup->targetEntry);
  uiEntrySetValue (altsetup->targetEntry, altsetup->target);
  uiwidgetp = uiEntryGetUIWidget (altsetup->targetEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (&hbox, uiwidgetp);
  uiEntrySetValidate (altsetup->targetEntry,
      altsetupValidateTarget, altsetup, UIENTRY_DELAYED);

  uiutilsUICallbackInit (&altsetup->callbacks [ALT_CB_TARGET_DIR],
      altsetupTargetDirDialog, altsetup, NULL);
  uibutton = uiCreateButton (
      &altsetup->callbacks [ALT_CB_TARGET_DIR],
      "", NULL);
  altsetup->buttons [ALT_BUTTON_TARGET_DIR] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget,
      /* CONTEXT: set up alternate: name (for shortcut) */
      _("Name"));
  uiBoxPackStart (&hbox, &uiwidget);

  uiEntryCreate (altsetup->nameEntry);
  uiEntrySetValue (altsetup->nameEntry, "BDJ4 B");
  uiwidgetp = uiEntryGetUIWidget (altsetup->nameEntry);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: set up alternate: checkbox: re-install alternate */
  uiCreateCheckButton (&altsetup->reinstWidget, _("Re-Install"),
      altsetup->reinstall);
  uiBoxPackStart (&hbox, &altsetup->reinstWidget);
  uiutilsUICallbackInit (&altsetup->reinstcb, altsetupCheckDirTarget, altsetup, NULL);
  uiToggleButtonSetCallback (&altsetup->reinstWidget, &altsetup->reinstcb);

  uiCreateLabel (&altsetup->feedbackMsg, "");
  uiLabelSetColor (&altsetup->feedbackMsg, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiBoxPackStart (&hbox, &altsetup->feedbackMsg);

  uiCreateHorizSeparator (&uiwidget);
  uiSeparatorSetColor (&uiwidget, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiBoxPackStart (&vbox, &uiwidget);

  /* button box */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uibutton = uiCreateButton (
      &altsetup->callbacks [ALT_CB_EXIT],
      /* CONTEXT: set up alternate: exits the altsetup */
      _("Exit"), NULL);
  altsetup->buttons [ALT_BUTTON_EXIT] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackEnd (&hbox, uiwidgetp);

  uiutilsUICallbackInit (&altsetup->callbacks [ALT_CB_START],
      altsetupSetupCallback, altsetup, NULL);
  uibutton = uiCreateButton (
      &altsetup->callbacks [ALT_CB_START],
      /* CONTEXT: set up alternate: start the set-up process */
      _("Start"), NULL);
  altsetup->buttons [ALT_BUTTON_START] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackEnd (&hbox, uiwidgetp);

  altsetup->disptb = uiTextBoxCreate (200, NULL);
  uiTextBoxSetReadonly (altsetup->disptb);
  uiTextBoxHorizExpand (altsetup->disptb);
  uiTextBoxVertExpand (altsetup->disptb);
  uiBoxPackStartExpand (&vbox, uiTextBoxGetScrolledWindow (altsetup->disptb));

  uiWidgetShowAll (&altsetup->window);
  altsetup->uiBuilt = true;
}

static int
altsetupMainLoop (void *udata)
{
  altsetup_t *altsetup = udata;

  uiUIProcessEvents ();

  uiEntryValidate (altsetup->targetEntry, false);

  if (altsetup->scrolltoend) {
    uiTextBoxScrollToEnd (altsetup->disptb);
    altsetup->scrolltoend = false;
    uiUIProcessEvents ();
    mssleep (10);
    uiUIProcessEvents ();
    /* go through the main loop once more */
    return TRUE;
  }

  switch (altsetup->instState) {
    case ALT_PRE_INIT: {
      altsetup->instState = ALT_PREPARE;
      break;
    }
    case ALT_PREPARE: {
      uiEntryValidate (altsetup->targetEntry, true);
      altsetup->instState = ALT_WAIT_USER;
      break;
    }
    case ALT_WAIT_USER: {
      /* do nothing */
      break;
    }
    case ALT_INIT: {
      altsetupInit (altsetup);
      break;
    }
    case ALT_MAKE_TARGET: {
      altsetupMakeTarget (altsetup);
      break;
    }
    case ALT_CHDIR: {
      altsetupChangeDir (altsetup);
      break;
    }
    case ALT_CREATE_DIRS: {
      altsetupCreateDirs (altsetup);

      logStart ("bdj4altsetup", "as",
          LOG_IMPORTANT | LOG_BASIC | LOG_MAIN | LOG_REDIR_INST);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "=== set up alternate started");
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "target: %s", altsetup->target);
      break;
    }
    case ALT_COPY_TEMPLATES: {
      altsetupCopyTemplates (altsetup);
      break;
    }
    case ALT_SETUP: {
      altsetupSetup (altsetup);
      break;
    }
    case ALT_CREATE_SHORTCUT: {
      altsetupCreateShortcut (altsetup);
      break;
    }
    case ALT_UPDATE_PROCESS_INIT: {
      altsetupUpdateProcessInit (altsetup);
      break;
    }
    case ALT_UPDATE_PROCESS: {
      altsetupUpdateProcess (altsetup);
      break;
    }
    case ALT_FINISH: {
      /* CONTEXT: set up alternate: status message */
      altsetupDisplayText (altsetup, "## ",  _("Setup complete."), true);
      altsetup->instState = ALT_PREPARE;
      break;
    }
    case ALT_EXIT: {
      break;
    }
  }

  return TRUE;
}

static bool
altsetupCheckDirTarget (void *udata)
{
  altsetup_t   *altsetup = udata;

  uiEntryValidate (altsetup->targetEntry, true);
  return UICB_CONT;
}

static int
altsetupValidateTarget (uientry_t *entry, void *udata)
{
  altsetup_t   *altsetup = udata;
  const char    *dir;
  bool          exists = false;
  bool          tbool;
  char          tbuff [MAXPATHLEN];
  int           rc = UIENTRY_OK;

  if (! altsetup->uiBuilt) {
    return UIENTRY_RESET;
  }

  dir = uiEntryGetValue (altsetup->targetEntry);
  tbool = uiToggleButtonIsActive (&altsetup->reinstWidget);
  altsetup->newinstall = false;
  altsetup->reinstall = tbool;

  exists = fileopIsDirectory (dir);

  strlcpy (tbuff, dir, sizeof (tbuff));
  strlcat (tbuff, "/data", sizeof (tbuff));
  if (! fileopIsDirectory (tbuff)) {
    exists = false;
  }

  if (exists) {
    if (tbool) {
      /* CONTEXT: set up alternate: message indicating the action that will be taken */
      snprintf (tbuff, sizeof (tbuff), _("Re-install existing alternate."));
      uiLabelSetText (&altsetup->feedbackMsg, tbuff);
    } else {
      /* CONTEXT: set up alternate: message indicating the action that will be taken */
      snprintf (tbuff, sizeof (tbuff), _("Updating existing alternate."));
      uiLabelSetText (&altsetup->feedbackMsg, tbuff);
    }
  } else {
    altsetup->newinstall = true;
    /* CONTEXT: set up alternate: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("New alternate folder."));
    uiLabelSetText (&altsetup->feedbackMsg, tbuff);
  }

  if (! *dir || ! fileopIsDirectory (dir)) {
    rc = UIENTRY_ERROR;
  }

  if (rc == UIENTRY_OK) {
    altsetupSetPaths (altsetup);
  }
  return rc;
}

static bool
altsetupTargetDirDialog (void *udata)
{
  altsetup_t *altsetup = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;

  selectdata = uiDialogCreateSelect (&altsetup->window,
      /* CONTEXT: set up alternate: dialog title for selecting location */
      _("Alternate Location"),
      uiEntryGetValue (altsetup->targetEntry), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    char        tbuff [MAXPATHLEN];

    strlcpy (tbuff, fn, sizeof (tbuff));
    /* validation gets called again upon set */
    uiEntrySetValue (altsetup->targetEntry, tbuff);
    free (fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected target loc: %s", altsetup->target);
  }
  free (selectdata);
  return UICB_CONT;
}

static bool
altsetupExitCallback (void *udata)
{
  altsetup_t   *altsetup = udata;

  altsetup->instState = ALT_EXIT;
  return UICB_CONT;
}

static bool
altsetupSetupCallback (void *udata)
{
  altsetup_t *altsetup = udata;

  if (altsetup->instState == ALT_WAIT_USER) {
    altsetup->instState = ALT_INIT;
  }
  return UICB_CONT;
}

static void
altsetupSetPaths (altsetup_t *altsetup)
{
  altsetupSetTargetDir (altsetup, uiEntryGetValue (altsetup->targetEntry));
}

static void
altsetupInit (altsetup_t *altsetup)
{
  altsetupSetPaths (altsetup);
  altsetup->reinstall = uiToggleButtonIsActive (&altsetup->reinstWidget);
  altsetup->instState = ALT_MAKE_TARGET;
}

static void
altsetupMakeTarget (altsetup_t *altsetup)
{
  diropMakeDir (altsetup->target);

  altsetup->instState = ALT_CHDIR;
}

static void
altsetupChangeDir (altsetup_t *altsetup)
{
  if (chdir (altsetup->target)) {
    altsetupFailWorkingDir (altsetup, altsetup->target);
    return;
  }

  if (altsetup->newinstall || altsetup->reinstall) {
    altsetup->instState = ALT_CREATE_DIRS;
  } else {
    altsetup->instState = ALT_UPDATE_PROCESS_INIT;
  }
}

static void
altsetupCreateDirs (altsetup_t *altsetup)
{
  /* CONTEXT: set up alternate: status message */
  altsetupDisplayText (altsetup, "-- ", _("Creating folder structure."), false);

  /* create the directories that are not included in the distribution */
  diropMakeDir ("data");
  /* this will create the directories necessary for the configs */
  bdjoptCreateDirectories ();
  diropMakeDir ("tmp");
  diropMakeDir ("http");
  /* there are profile specific image files */
  diropMakeDir ("img/profile00");

  altsetup->instState = ALT_COPY_TEMPLATES;
}

static void
altsetupCopyTemplates (altsetup_t *altsetup)
{
  char            dir [MAXPATHLEN];
  char            from [MAXPATHLEN];
  char            to [MAXPATHLEN];
  char            tbuff [MAXPATHLEN];
  const char      *fname;
  pathinfo_t      *pi;
  slist_t         *dirlist;
  slistidx_t      iteridx;
  datafile_t      *srdf;
  datafile_t      *qddf;
  datafile_t      *autodf;
  slist_t         *renamelist;


  /* CONTEXT: set up alternate: status message */
  altsetupDisplayText (altsetup, "-- ", _("Copying template files."), false);

  if (chdir (altsetup->target)) {
    altsetupFailWorkingDir (altsetup, altsetup->target);
    return;
  }

  renamelist = NULL;

  pathbldMakePath (tbuff, sizeof (tbuff),
      "localized-sr", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_INST);
  srdf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL, tbuff, NULL, 0);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "localized-auto", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_INST);
  autodf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL, tbuff, NULL, 0);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "localized-qd", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_INST);
  qddf = datafileAllocParse ("loc-qd", DFTYPE_KEY_VAL, tbuff, NULL, 0);

  pathbldMakePath (dir, sizeof (dir),
      "", "", PATHBLD_MP_DIR_TEMPLATE);
  dirlist = dirlistBasicDirList (dir, NULL);
  slistStartIterator (dirlist, &iteridx);
  while ((fname = slistIterateKey (dirlist, &iteridx)) != NULL) {
    if (strcmp (fname, "qrcode") == 0) {
      continue;
    }
    if (strcmp (fname, "qrcode.html") == 0) {
      continue;
    }
    if (strcmp (fname, "html-list.txt") == 0) {
      continue;
    }
    if (strcmp (fname, "helpdata.txt") == 0) {
      continue;
    }
    if (strcmp (fname, "volintfc.txt") == 0) {
      continue;
    }
    if (strcmp (fname, "playerintfc.txt") == 0) {
      continue;
    }

    strlcpy (from, fname, sizeof (from));

    if (strcmp (fname, "bdj-flex-dark.html") == 0) {
      pathbldMakePath (to, sizeof (to),
          "bdj4remote.html", "", PATHBLD_MP_DREL_HTTP);
      altsetupTemplateCopy (dir, from, to);
      continue;
    }
    if (strcmp (fname, "mobilemq.html") == 0) {
      pathbldMakePath (to, sizeof (to),
          "mobilemq.html", "", PATHBLD_MP_DREL_HTTP);
      altsetupTemplateCopy (dir, from, to);
      continue;
    }

    pi = pathInfo (fname);
    if (pathInfoExtCheck (pi, ".html")) {
      free (pi);
      continue;
    }

    if (pathInfoExtCheck (pi, ".crt")) {
      pathbldMakePath (to, sizeof (to),
          fname, "", PATHBLD_MP_DREL_HTTP);
    } else if (pathInfoExtCheck (pi, ".svg")) {
      pathbldMakePath (to, sizeof (to),
          fname, "", PATHBLD_MP_DIR_IMG | PATHBLD_MP_USEIDX);
    } else if (strncmp (fname, "bdjconfig", 9) == 0) {
      snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->blen, pi->basename);
      if (pathInfoExtCheck (pi, ".g")) {
        pathbldMakePath (to, sizeof (to),
            tbuff, "", PATHBLD_MP_DREL_DATA);
      } else if (pathInfoExtCheck (pi, ".p")) {
        pathbldMakePath (to, sizeof (to),
            tbuff, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
      } else if (pathInfoExtCheck (pi, ".txt")) {
        pathbldMakePath (to, sizeof (to),
            fname, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
      } else if (pathInfoExtCheck (pi, ".m")) {
        pathbldMakePath (to, sizeof (to),
            tbuff, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME);
      } else if (pathInfoExtCheck (pi, ".mp")) {
        pathbldMakePath (to, sizeof (to),
            tbuff, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
      } else {
        /* unknown extension */
        free (pi);
        continue;
      }
    } else if (pathInfoExtCheck (pi, BDJ4_CONFIG_EXT) ||
        pathInfoExtCheck (pi, BDJ4_SEQUENCE_EXT) ||
        pathInfoExtCheck (pi, BDJ4_PL_DANCE_EXT) ||
        pathInfoExtCheck (pi, BDJ4_PLAYLIST_EXT) ) {

      renamelist = NULL;
      if (strncmp (pi->basename, "automatic", pi->blen) == 0) {
        renamelist = datafileGetList (autodf);
      }
      if (strncmp (pi->basename, "standardrounds", pi->blen) == 0) {
        renamelist = datafileGetList (srdf);
      }
      if (strncmp (pi->basename, "QueueDance", pi->blen) == 0) {
        renamelist = datafileGetList (qddf);
      }

      strlcpy (tbuff, fname, sizeof (tbuff));
      if (renamelist != NULL) {
        char    *tval;

        tval = slistGetStr (renamelist, sysvarsGetStr (SV_LOCALE_SHORT));
        if (tval != NULL) {
          snprintf (tbuff, sizeof (tbuff), "%s%.*s", tval, (int) pi->elen,
              pi->extension);
        }
      }

      strlcpy (from, tbuff, sizeof (from));
      if (strncmp (pi->basename, "ds-", 3) == 0) {
        pathbldMakePath (to, sizeof (to),
            fname, "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
      } else {
        pathbldMakePath (to, sizeof (to),
            fname, "", PATHBLD_MP_DREL_DATA);
        snprintf (to, sizeof (to), "data/%s", tbuff);
      }
    } else {
      /* unknown extension */
      free (pi);
      continue;
    }

    altsetupTemplateCopy (dir, from, to);

    free (pi);
  }
  slistFree (dirlist);

  snprintf (dir, sizeof (dir), "%s/img", altsetup->maindir);

  strlcpy (from, "favicon.ico", sizeof (from));
  pathbldMakePath (to, sizeof (to),
      "favicon.ico", "", PATHBLD_MP_DREL_HTTP);
  altsetupTemplateCopy (dir, from, to);

  strlcpy (from, "led_on.svg", sizeof (from));
  pathbldMakePath (to, sizeof (to),
      "led_on", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DREL_HTTP);
  altsetupTemplateCopy (dir, from, to);

  strlcpy (from, "led_off.svg", sizeof (from));
  pathbldMakePath (to, sizeof (to),
      "led_off", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DREL_HTTP);
  altsetupTemplateCopy (dir, from, to);

  strlcpy (from, "ballroomdj4.svg", sizeof (from));
  pathbldMakePath (to, sizeof (to),
      "ballroomdj", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DREL_HTTP);
  altsetupTemplateCopy (dir, from, to);

  snprintf (from, sizeof (from), "%s/img/mrc", altsetup->maindir);
  snprintf (to, sizeof (to), "http/mrc");
  *tbuff = '\0';
  if (isWindows ()) {
    pathWinPath (from, sizeof (from));
    pathWinPath (to, sizeof (to));
    snprintf (tbuff, sizeof (tbuff), "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl \"%s\" \"%s\"",
        from, to);
  } else {
    snprintf (tbuff, sizeof (tbuff), "cp -r '%s' '%s'", from, "http");
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "copy files: %s", tbuff);
  (void) ! system (tbuff);

  datafileFree (srdf);
  datafileFree (autodf);
  datafileFree (qddf);

  altsetup->instState = ALT_SETUP;
}

static void
altsetupSetup (altsetup_t *altsetup)
{
  char    buff [MAXPATHLEN];
  char    tbuff [MAXPATHLEN];
  FILE    *fh;
  char    str [40];
  int     altcount;
  int     baseport;

  /* CONTEXT: set up alternate: status message */
  altsetupDisplayText (altsetup, "-- ", _("Initial Setup."), false);

  /* the altcount.txt file should only exist for the initial installation */
  pathbldMakePath (buff, sizeof (buff),
      ALT_COUNT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (fileopFileExists (buff)) {
    fileopDelete (buff);
  }

  pathbldMakePath (buff, sizeof (buff),
      "data/altcount", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_MAIN);

  /* read the current altcount */
  fh = fopen (buff, "r");
  (void) ! fgets (str, sizeof (str), fh);
  stringTrim (str);
  fclose (fh);
  altcount = atoi (str);

  /* update */
  ++altcount;
  snprintf (str, sizeof (str), "%d\n", altcount);

  /* write the new altcount */
  fh = fopen (buff, "w");
  fputs (str, fh);
  fclose (fh);

  /* calculate the new base port */
  baseport = sysvarsGetNum (SVL_BASEPORT);
  baseport += altcount * BDJOPT_MAX_PROFILES * (int) bdjvarsGetNum (BDJVL_NUM_PORTS);
  snprintf (str, sizeof (str), "%d\n", baseport);

  /* write the new base port out */
  pathbldMakePath (buff, sizeof (buff),
      BASE_PORT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  fh = fopen (buff, "w");
  fputs (str, fh);
  fclose (fh);

  /* create the symlink for the bdj4 executable */
  if (isWindows ()) {
    /* handled by the desktop shortcut */
  } else {
#if _lib_symlink
    diropMakeDir ("bin");

    pathbldMakePath (buff, sizeof (buff),
        "bdj4", "", PATHBLD_MP_DIR_EXEC);
    (void) ! symlink (buff, "bin/bdj4");
#endif
  }

  /* create the link files that point to the volreg.txt and lock file */
  pathbldMakePath (buff, sizeof (buff),
      VOLREG_FN, BDJ4_LINK_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "data/volreg", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_MAIN);
  fh = fopen (buff, "w");
  fputs (tbuff, fh);
  fputs ("\n", fh);
  fclose (fh);

  pathbldMakePath (buff, sizeof (buff),
      "volreglock", BDJ4_LINK_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "tmp/volreg", BDJ4_LOCK_EXT, PATHBLD_MP_DIR_MAIN);
  fh = fopen (buff, "w");
  fputs (tbuff, fh);
  fputs ("\n", fh);
  fclose (fh);

  altsetup->instState = ALT_CREATE_SHORTCUT;
}

static void
altsetupCreateShortcut (altsetup_t *altsetup)
{
  const char  *name;

  if (chdir (altsetup->maindir)) {
    altsetupFailWorkingDir (altsetup, altsetup->maindir);
    return;
  }

  /* CONTEXT: set up alternate: status message */
  altsetupDisplayText (altsetup, "-- ", _("Creating shortcut."), false);
  name = uiEntryGetValue (altsetup->nameEntry);
  if (name == NULL || ! *name) {
    name = "BDJ4-alt";
  }
  instutilCreateShortcut (name, altsetup->maindir, altsetup->target, 0);

  altsetup->instState = ALT_UPDATE_PROCESS_INIT;
}

static void
altsetupUpdateProcessInit (altsetup_t *altsetup)
{
  char  buff [MAXPATHLEN];

  if (chdir (altsetup->target)) {
    altsetupFailWorkingDir (altsetup, altsetup->target);
    return;
  }

  /* CONTEXT: set up alternate: status message */
  snprintf (buff, sizeof (buff), _("Updating %s."), BDJ4_LONG_NAME);
  altsetupDisplayText (altsetup, "-- ", buff, false);
  altsetup->instState = ALT_UPDATE_PROCESS;
}

static void
altsetupUpdateProcess (altsetup_t *altsetup)
{
  char  tbuff [MAXPATHLEN];
  int   targc = 0;
  const char  *targv [10];

  snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4%s",
      altsetup->maindir, sysvarsGetStr (SV_OS_EXEC_EXT));
  targv [targc++] = tbuff;
  targv [targc++] = "--bdj4updater";

  /* only need to run the 'newinstall' update process when the template */
  /* files have been copied */
  if (altsetup->newinstall || altsetup->reinstall) {
    targv [targc++] = "--newinstall";
  }
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);

  altsetup->instState = ALT_FINISH;
}

/* support routines */

static void
altsetupCleanup (altsetup_t *altsetup)
{
  if (altsetup->target != NULL) {
    for (int i = 0; i < ALT_BUTTON_MAX; ++i) {
      uiButtonFree (altsetup->buttons [i]);
    }
    uiEntryFree (altsetup->nameEntry);
    uiEntryFree (altsetup->targetEntry);
    free (altsetup->target);
  }
}

static void
altsetupDisplayText (altsetup_t *altsetup, char *pfx, char *txt, bool bold)
{
  if (bold) {
    uiTextBoxAppendBoldStr (altsetup->disptb, pfx);
    uiTextBoxAppendBoldStr (altsetup->disptb, txt);
    uiTextBoxAppendBoldStr (altsetup->disptb, "\n");
  } else {
    uiTextBoxAppendStr (altsetup->disptb, pfx);
    uiTextBoxAppendStr (altsetup->disptb, txt);
    uiTextBoxAppendStr (altsetup->disptb, "\n");
  }
  altsetup->scrolltoend = true;
}

static void
altsetupTemplateCopy (const char *dir, const char *from, const char *to)
{
  char      *localetmpldir;
  char      tbuff [MAXPATHLEN];

  localetmpldir = sysvarsGetStr (SV_LOCALE);
  snprintf (tbuff, sizeof (tbuff), "%s/%s/%s",
      dir, localetmpldir, from);
  logMsg (LOG_INSTALL, LOG_MAIN, "check file: %s", tbuff);
  if (fileopFileExists (tbuff)) {
    logMsg (LOG_INSTALL, LOG_MAIN, "   found");
    from = tbuff;
  } else {
    localetmpldir = sysvarsGetStr (SV_LOCALE_SHORT);
    snprintf (tbuff, sizeof (tbuff), "%s/%s/%s",
        dir, localetmpldir, from);
    logMsg (LOG_INSTALL, LOG_MAIN, "check file: %s", tbuff);
    if (fileopFileExists (tbuff)) {
      logMsg (LOG_INSTALL, LOG_MAIN, "   found");
      from = tbuff;
    } else {
      snprintf (tbuff, sizeof (tbuff), "%s/%s", dir, from);
      from = tbuff;
    }
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "- copy: %s", from);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "    to: %s", to);
  filemanipBackup (to, 1);
  filemanipCopy (from, to);
}

static void
altsetupFailWorkingDir (altsetup_t *altsetup, const char *dir)
{
  fprintf (stderr, "Unable to set working dir: %s\n", dir);
  /* CONTEXT: set up alternate: failure message */
  altsetupDisplayText (altsetup, "", _("Error: Unable to set working folder."), false);
  /* CONTEXT: set up alternate: status message */
  altsetupDisplayText (altsetup, " * ", _("Process aborted."), false);
  altsetup->instState = ALT_WAIT_USER;
}

static void
altsetupSetTargetDir (altsetup_t *altsetup, const char *fn)
{
  dataFree (altsetup->target);
  altsetup->target = strdup (fn);
  pathNormPath (altsetup->target, strlen (altsetup->target));
}

