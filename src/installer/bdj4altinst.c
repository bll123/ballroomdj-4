/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <signal.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "callback.h"
#include "datafile.h"
#include "dirop.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "instutil.h"
#include "localeutil.h"
#include "log.h"
#include "mdebug.h"
#include "osdirutil.h"
#include "osprocess.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathinfo.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "templateutil.h"
#include "tmutil.h"
#include "ui.h"
#include "uiclass.h"
#include "uiutils.h"
#include "validate.h"

/* setup states */
typedef enum {
  ALT_PRE_INIT,
  ALT_PREPARE,
  ALT_WAIT_USER,
  ALT_INIT,
  ALT_SAVE_TARGET,
  ALT_MAKE_TARGET,
  ALT_CHDIR,
  ALT_CREATE_DIRS,
  ALT_COPY_TEMPLATES,
  ALT_SETUP,
  ALT_CREATE_LAUNCHER,
  ALT_FINALIZE,
  ALT_UPDATE_PROCESS_INIT,
  ALT_UPDATE_PROCESS,
  ALT_FINISH,
  ALT_EXIT,
} altinststate_t;

enum {
  ALT_CB_TARGET_DIR,
  ALT_CB_EXIT,
  ALT_CB_START,
  ALT_CB_REINST,
  ALT_CB_MAX,
};

enum {
  ALT_TARGET,
};

enum {
  ALT_W_BUTTON_TARGET_DIR,
  ALT_W_BUTTON_EXIT,
  ALT_W_BUTTON_START,
  ALT_W_WINDOW,
  ALT_W_ERROR_MSG,
  ALT_W_REINST,
  ALT_W_FEEDBACK_MSG,
  ALT_W_STATUS_DISP,
  ALT_W_TARGET,
  ALT_W_NAME,
  ALT_W_MAX,
};

typedef struct {
  altinststate_t  instState;
  altinststate_t  lastInstState;            // debugging
  callback_t      *callbacks [ALT_CB_MAX];
  char            oldversion [MAXPATHLEN];
  char            *basedir;
  char            *target;
  char            datatopdir [MAXPATHLEN];
  char            rundir [MAXPATHLEN];      // installation dir with macospfx
  const char      *launchname;
  const char      *macospfx;
  char            *maindir;
  const char      *hostname;
  const char      *home;
  char            *name;
  int             altidx;
  /* conversion */
  uiwcont_t       *wcont [ALT_W_MAX];
  /* ati */
  char            ati [40];
  int             atiselect;
  /* flags */
  bool            bdjoptloaded : 1;
  bool            firstinstall : 1;
  bool            guienabled : 1;
  bool            localespecified : 1;
  bool            newinstall : 1;
  bool            quiet : 1;
  bool            reinstall : 1;
  bool            scrolltoend : 1;
  bool            targetexists : 1;
  bool            testregistration : 1;
  bool            uiBuilt : 1;
  bool            unattended : 1;
  bool            updateinstall : 1;
  bool            verbose : 1;
} altinst_t;

static void altinstBuildUI (altinst_t *altinst);
static int  altinstMainLoop (void *udata);
static bool altinstExitCallback (void *udata);
static bool altinstReinstallCBHandler (void *udata);
static bool altinstTargetDirDialog (void *udata);
static int  altinstValidateTarget (uiwcont_t *entry, const char *label, void *udata);
static int  altinstValidateProcessTarget (altinst_t *altinst, const char *dir);
static int  altinstValidateName (uiwcont_t *entry, const char *label, void *udata);
static void altinstTargetFeedbackMsg (altinst_t *altinst);
static bool altinstSetupCallback (void *udata);
static void altinstSetPaths (altinst_t *altinst);

static void altinstPrepare (altinst_t *altinst);
static void altinstInit (altinst_t *altinst);
static void altinstSaveTargetDir (altinst_t *altinst);
static void altinstMakeTarget (altinst_t *altinst);
static void altinstChangeDir (altinst_t *altinst);
static void altinstCreateDirs (altinst_t *altinst);
static void altinstCopyTemplates (altinst_t *altinst);
static void altinstSetup (altinst_t *altinst);
static void altinstCreateLauncher (altinst_t *altinst);
static void altinstUpdateProcessInit (altinst_t *altinst);
static void altinstUpdateProcess (altinst_t *altinst);
static void altinstFinalize (altinst_t *altinst);

static void altinstCleanup (altinst_t *altinst);
static void altinstDisplayText (altinst_t *altinst, char *pfx, char *txt, bool bold);
static void altinstFailWorkingDir (altinst_t *altinst, const char *dir, const char *tag);
static void altinstSetTargetDir (altinst_t *altinst, const char *fn);
static void altinstLoadBdjOpt (altinst_t *altinst);
static void altinstBuildTarget (altinst_t *altinst, char *buff, size_t sz, const char *nm);
static void altinstSigHandler (int sig);
static void altinstSetTargetEntry (altinst_t *altinst, const char *fn);

int
main (int argc, char *argv[])
{
  altinst_t     altinst;
  char          tbuff [MAXPATHLEN];
  FILE          *fh;
  uint32_t      flags;
  const char    *tmp;

#if BDJ4_MEM_DEBUG
  mdebugInit ("alt");
#endif

  altinst.instState = ALT_PRE_INIT;
  altinst.lastInstState = ALT_PRE_INIT;
  altinst.basedir = mdstrdup ("");
  altinst.target = mdstrdup ("");
  altinst.launchname = "bdj4";
  altinst.macospfx = "";
  strcpy (altinst.oldversion, "");
  altinst.maindir = NULL;
  altinst.home = NULL;
  altinst.name = NULL;
  altinst.hostname = NULL;
  altinst.bdjoptloaded = false;
  altinst.firstinstall = false;
  altinst.guienabled = true;
  altinst.localespecified = false;
  altinst.newinstall = false;
  altinst.quiet = false;
  altinst.reinstall = false;
  altinst.scrolltoend = false;
  altinst.targetexists = false;
  altinst.testregistration = false;
  altinst.uiBuilt = false;
  altinst.unattended = false;
  altinst.updateinstall = false;
  altinst.verbose = false;
  altinst.altidx = 1;
  for (int i = 0; i < ALT_W_MAX; ++i) {
    altinst.wcont [i] = NULL;
  }
  for (int i = 0; i < ALT_CB_MAX; ++i) {
    altinst.callbacks [i] = NULL;
  }
  tbuff [0] = '\0';

  osSetStandardSignals (altinstSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_LOCK | BDJ4_INIT_NO_LOG;
  /* if the data-top-dir exists, the data files will be loaded */
  bdj4startup (argc, argv, NULL, "alt", ROUTE_ALTINST, &flags);
  if ((flags & BDJ4_ARG_INST_REINSTALL) == BDJ4_ARG_INST_REINSTALL) {
    altinst.reinstall = true;
  }
  if ((flags & BDJ4_ARG_INST_UNATTENDED) == BDJ4_ARG_INST_UNATTENDED) {
    altinst.unattended = true;
    altinst.guienabled = false;
#if _define_SIGCHLD
    osDefaultSignal (SIGCHLD);
#endif
  }
  if ((flags & BDJ4_ARG_VERBOSE) == BDJ4_ARG_VERBOSE) {
    altinst.verbose = true;
  }
  if ((flags & BDJ4_ARG_QUIET) == BDJ4_ARG_QUIET) {
    altinst.quiet = true;
  }

  if (isMacOS ()) {
    altinst.launchname = "bdj4g";
    altinst.macospfx = MACOS_APP_PREFIX;
  }

  /* the first time the name is set, append the dev status */
  snprintf (tbuff, sizeof (tbuff), "%s%s%s",
      BDJ4_NAME, "alt", sysvarsGetStr (SV_BDJ4_DEVELOPMENT));
  altinst.name = mdstrdup (tbuff);
  altinst.maindir = sysvarsGetStr (SV_BDJ4_DIR_MAIN);
  altinst.home = sysvarsGetStr (SV_HOME);
  altinst.hostname = sysvarsGetStr (SV_HOSTNAME);
  dataFree (altinst.basedir);
  altinst.basedir = mdstrdup (altinst.home);

  tmp = bdjvarsGetStr (BDJV_INST_TARGET);
  if (tmp != NULL) {
    /* from the command line */
    altinstSetTargetDir (&altinst, tmp);
  } else {
    char    tfn [MAXPATHLEN];

    altinstBuildTarget (&altinst, tbuff, sizeof (tbuff), altinst.name);

    /* if the altinstdirNN.txt file exists, use it */
    snprintf (tfn, sizeof (tfn), "%s/%s%s%02d%s",
        sysvarsGetStr (SV_DIR_CONFIG), ALT_INST_PATH_FN,
        sysvarsGetStr (SV_BDJ4_DEVELOPMENT), 1, BDJ4_CONFIG_EXT);
    fh = fileopOpen (tfn, "r");
    if (fh != NULL) {
      pathinfo_t    *pi;

      (void) ! fgets (tbuff, sizeof (tbuff), fh);
      stringTrim (tbuff);
      mdextfclose (fh);
      fclose (fh);

      pi = pathInfo (tbuff);
      dataFree (altinst.name);
      altinst.name = mdmalloc (pi->flen + 1);
      strlcpy (altinst.name, pi->filename, pi->flen + 1);
      altinst.name [pi->flen] = '\0';
      pathInfoFree (pi);
    }

    altinstSetTargetDir (&altinst, tbuff);
  }

  /* the altcount.txt lives in the configuration dir */
  if (! fileopFileExists (sysvarsGetStr (SV_FILE_ALTCOUNT))) {
    altinst.firstinstall = true;
  }

  if (altinst.firstinstall && sysvarsGetNum (SVL_IS_READONLY)) {
    dataFree (altinst.name);
    altinst.name = mdstrdup (BDJ4_NAME);
  }

  if (osChangeDir (altinst.maindir) < 0) {
    fprintf (stderr, "ERR: Unable to chdir to %s\n", altinst.maindir);
  }

  if (bdjoptGetStr (OPT_P_UI_ACCENT_COL) == NULL) {
    bdjoptSetStr (OPT_P_UI_ACCENT_COL, INST_HL_COLOR);
  }
  if (bdjoptGetStr (OPT_P_UI_ERROR_COL) == NULL) {
    bdjoptSetStr (OPT_P_UI_ERROR_COL, "#ff2222");
  }

  if (altinst.guienabled) {
    uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));

    uiSetUICSS (uiutilsGetCurrentFont (),
        uiutilsGetListingFont (),
        bdjoptGetStr (OPT_P_UI_ACCENT_COL),
        bdjoptGetStr (OPT_P_UI_ERROR_COL),
      bdjoptGetStr (OPT_P_UI_MARK_COL),
      bdjoptGetStr (OPT_P_UI_ROWSEL_COL),
      bdjoptGetStr (OPT_P_UI_ROW_HL_COL));

    altinstBuildUI (&altinst);
    osuiFinalize ();
  }

  while (altinst.instState != ALT_EXIT) {
    if (altinst.instState != altinst.lastInstState) {
      if (altinst.verbose && ! altinst.quiet) {
        fprintf (stderr, "state: %d\n", altinst.instState);
      }
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "state: %d", altinst.instState);
      altinst.lastInstState = altinst.instState;
    }
    altinstMainLoop (&altinst);
    mssleep (10);
  }

  if (altinst.guienabled) {
    /* process any final events */
    uiUIProcessEvents ();
    uiCloseWindow (altinst.wcont [ALT_W_WINDOW]);
    uiCleanup ();
  }

  altinstCleanup (&altinst);
  bdj4shutdown (ROUTE_ALTINST, NULL);
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

static void
altinstBuildUI (altinst_t *altinst)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp;
  char          tbuff [100];
  char          imgbuff [MAXPATHLEN];
  uiutilsaccent_t accent;

  strlcpy (imgbuff, "img/bdj4_icon_inst.png", sizeof (imgbuff));
  osuiSetIcon (imgbuff);

  strlcpy (imgbuff, "img/bdj4_icon_inst.svg", sizeof (imgbuff));
  /* CONTEXT: alternate installation: window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Installer"), BDJ4_NAME);
  altinst->callbacks [ALT_CB_EXIT] = callbackInit (
      altinstExitCallback, altinst, NULL);
  altinst->wcont [ALT_W_WINDOW] = uiCreateMainWindow (
      altinst->callbacks [ALT_CB_EXIT], tbuff, imgbuff);
  uiWindowSetDefaultSize (altinst->wcont [ALT_W_WINDOW], 1000, 600);

  vbox = uiCreateVertBox ();
  uiWindowPackInWindow (altinst->wcont [ALT_W_WINDOW], vbox);
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);

  uiutilsAddProfileColorDisplay (vbox, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.cbox);

  /* begin line : status message */

  uiwidgetp = uiCreateLabel ("");
  uiWidgetAddClass (uiwidgetp, ERROR_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  altinst->wcont [ALT_W_ERROR_MSG] = uiwidgetp;

  /* begin line : instructions */

  uiwidgetp = uiCreateLabel (
      /* CONTEXT: alternate installation: installation instructions */
      _("Enter the name of the new installation."));
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* begin line : name */

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: alternate installation: name (for shortcut) */
  uiwidgetp = uiCreateColonLabel (_("Name"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (30, 30);
  uiEntrySetValue (uiwidgetp, altinst->name);
  uiBoxPackStart (hbox, uiwidgetp);
  altinst->wcont [ALT_W_NAME] = uiwidgetp;

  uiwcontFree (hbox);

  /* begin line : target entry */

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiEntryInit (80, MAXPATHLEN);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  altinst->wcont [ALT_W_TARGET] = uiwidgetp;
  if (isMacOS ()) {
    uiWidgetSetState (uiwidgetp, UIWIDGET_DISABLE);
  }
  altinstSetTargetEntry (altinst, altinst->target);

  altinst->callbacks [ALT_CB_TARGET_DIR] = callbackInit (
      altinstTargetDirDialog, altinst, NULL);
  uiwidgetp = uiCreateButton (
      altinst->callbacks [ALT_CB_TARGET_DIR],
      "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 0);
  altinst->wcont [ALT_W_BUTTON_TARGET_DIR] = uiwidgetp;
  if (isMacOS ()) {
    uiWidgetSetState (uiwidgetp, UIWIDGET_DISABLE);
  }

  uiwcontFree (hbox);

  /* begin line : re-install checkbox */

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: alternate installation: checkbox: re-install */
  altinst->wcont [ALT_W_REINST] = uiCreateCheckButton (_("Re-Install"),
      altinst->reinstall);
  uiBoxPackStart (hbox, altinst->wcont [ALT_W_REINST]);
  altinst->callbacks [ALT_CB_REINST] = callbackInit (
      altinstReinstallCBHandler, altinst, NULL);
  uiToggleButtonSetCallback (altinst->wcont [ALT_W_REINST], altinst->callbacks [ALT_CB_REINST]);

  altinst->wcont [ALT_W_FEEDBACK_MSG] = uiCreateLabel ("");
  uiWidgetAddClass (altinst->wcont [ALT_W_FEEDBACK_MSG], ACCENT_CLASS);
  uiBoxPackStart (hbox, altinst->wcont [ALT_W_FEEDBACK_MSG]);

  uiwcontFree (hbox);

  /* begin line : button box */

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateButton (
      altinst->callbacks [ALT_CB_EXIT],
      /* CONTEXT: alternate installation: exits the alternate installer */
      _("Exit"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  altinst->wcont [ALT_W_BUTTON_EXIT] = uiwidgetp;

  altinst->callbacks [ALT_CB_START] = callbackInit (
      altinstSetupCallback, altinst, NULL);
  uiwidgetp = uiCreateButton (
      altinst->callbacks [ALT_CB_START],
      /* CONTEXT: alternate installation: install BDJ4 in the alternate location */
      _("Install"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  altinst->wcont [ALT_W_BUTTON_START] = uiwidgetp;

  uiwidgetp = uiTextBoxCreate (200, NULL);
  uiTextBoxSetReadonly (uiwidgetp);
  uiTextBoxHorizExpand (uiwidgetp);
  uiTextBoxVertExpand (uiwidgetp);
  uiBoxPackStartExpand (vbox, uiwidgetp);
  altinst->wcont [ALT_W_STATUS_DISP] = uiwidgetp;

  uiWidgetShowAll (altinst->wcont [ALT_W_WINDOW]);
  altinst->uiBuilt = true;

  uiEntrySetValidate (altinst->wcont [ALT_W_TARGET], "",
      altinstValidateTarget, altinst, UIENTRY_DELAYED);
  /* CONTEXT: alternate installation: name (for shortcut) */
  uiEntrySetValidate (altinst->wcont [ALT_W_NAME], _("Name"),
      altinstValidateName, altinst, UIENTRY_IMMEDIATE);

  uiwcontFree (vbox);
  uiwcontFree (hbox);
}

static int
altinstMainLoop (void *udata)
{
  altinst_t *altinst = udata;

  if (altinst->guienabled) {
    uiUIProcessEvents ();

    uiEntryValidate (altinst->wcont [ALT_W_TARGET], false);

    if (altinst->scrolltoend) {
      uiTextBoxScrollToEnd (altinst->wcont [ALT_W_STATUS_DISP]);
      altinst->scrolltoend = false;
      uiUIProcessWaitEvents ();
      /* go through the main loop once more */
      return true;
    }
  }

  switch (altinst->instState) {
    case ALT_PRE_INIT: {
      altinst->instState = ALT_PREPARE;
      break;
    }
    case ALT_PREPARE: {
      altinstPrepare (altinst);
      break;
    }
    case ALT_WAIT_USER: {
      /* do nothing */
      if (! altinst->guienabled) {
        altinst->instState = ALT_INIT;
      }
      break;
    }
    case ALT_INIT: {
      altinstInit (altinst);
      break;
    }
    case ALT_SAVE_TARGET: {
      altinstSaveTargetDir (altinst);
      break;
    }
    case ALT_MAKE_TARGET: {
      altinstMakeTarget (altinst);
      break;
    }
    case ALT_CHDIR: {
      altinstChangeDir (altinst);
      break;
    }
    case ALT_CREATE_DIRS: {
      altinstCreateDirs (altinst);

      /* after change-dir and create-dir, the current-dir is still set */
      /* to the data-top-dir */
      logStart ("bdj4altinst", "alt",
          LOG_IMPORTANT | LOG_BASIC | LOG_INFO | LOG_REDIR_INST);
      logStartProgram ("alternate-setup");
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "target: %s", altinst->target);
      break;
    }
    case ALT_COPY_TEMPLATES: {
      altinstCopyTemplates (altinst);
      break;
    }
    case ALT_SETUP: {
      altinstSetup (altinst);
      break;
    }
    case ALT_CREATE_LAUNCHER: {
      altinstCreateLauncher (altinst);
      break;
    }
    case ALT_FINALIZE: {
      altinstFinalize (altinst);
      break;
    }
    case ALT_UPDATE_PROCESS_INIT: {
      altinstUpdateProcessInit (altinst);
      break;
    }
    case ALT_UPDATE_PROCESS: {
      altinstUpdateProcess (altinst);
      break;
    }
    case ALT_FINISH: {
      /* CONTEXT: alternate installation: status message */
      altinstDisplayText (altinst, INST_DISP_FIN, _("Setup complete."), true);
      altinst->instState = ALT_PREPARE;
      if (altinst->unattended) {
        altinst->instState = ALT_EXIT;
      }
      break;
    }
    case ALT_EXIT: {
      break;
    }
  }

  return true;
}

static bool
altinstReinstallCBHandler (void *udata)
{
  altinst_t    *altinst = udata;
  int           nval;

  nval = uiToggleButtonIsActive (altinst->wcont [ALT_W_REINST]);
  altinst->reinstall = nval;
  altinstTargetFeedbackMsg (altinst);
  return UICB_CONT;
}

static int
altinstValidateTarget (uiwcont_t *entry, const char *label, void *udata)
{
  altinst_t     *altinst = udata;
  const char    *dir;
  char          tbuff [MAXPATHLEN];
  int           rc = UIENTRY_ERROR;

  if (! altinst->guienabled) {
    return UIENTRY_ERROR;
  }

  if (! altinst->uiBuilt) {
    return UIENTRY_RESET;
  }

  dir = uiEntryGetValue (altinst->wcont [ALT_W_TARGET]);
  strlcpy (tbuff, dir, sizeof (tbuff));
  pathNormalizePath (tbuff, sizeof (tbuff));
  if (strcmp (tbuff, altinst->target) != 0) {
    rc = altinstValidateProcessTarget (altinst, tbuff);
  } else {
    rc = UIENTRY_OK;
  }

  return rc;
}

static int
altinstValidateProcessTarget (altinst_t *altinst, const char *dir)
{
  int         rc = UIENTRY_ERROR;
  bool        exists = false;
  bool        found = false;
  char        tbuff [MAXPATHLEN];

  if (fileopIsDirectory (dir)) {
    exists = true;
    found = instutilCheckForExistingInstall (dir, altinst->macospfx);
    if (! found) {
      strlcpy (tbuff, dir, sizeof (tbuff));
      if (altinst->firstinstall) {
        instutilAppendNameToTarget (tbuff, sizeof (tbuff), NULL, true);
      }
      exists = fileopIsDirectory (tbuff);
      if (exists) {
        found = instutilCheckForExistingInstall (tbuff, altinst->macospfx);
        if (found) {
          dir = tbuff;
        }
      }
    }

    /* do not try to overwrite an existing standard installation */
    if (exists && found &&
        ! instutilIsStandardInstall (dir, altinst->macospfx)) {
      /* this will be a re-install or an update */
      rc = UIENTRY_OK;
    }
  } else {
    pathinfo_t    *pi;
    char          tmp [MAXPATHLEN];

    exists = false;
    found = false;
    rc = UIENTRY_OK;

    /* the path doesn't exist, which is ok for a new installation */
    /* check the path up to that point and make sure it is valid */
    pi = pathInfo (dir);
    if (pi->dlen > 0) {
      pathInfoGetDir (pi, tmp, sizeof (tmp));
      if (! fileopIsDirectory (tmp)) {
        rc = UIENTRY_ERROR;
      }
    }
    pathInfoFree (pi);
  }

  altinst->newinstall = false;
  altinst->updateinstall = false;
  altinst->targetexists = false;
  if (exists) {
    altinst->targetexists = true;
  }

  if (rc == UIENTRY_OK) {
    /* set the target directory information */
    /* note that 'dir' may be invalid after this call */
    altinstSetTargetDir (altinst, dir);
    if (exists) {
      if (found) {
        altinst->newinstall = false;
        altinst->updateinstall = true;
      }
    }
    if (! exists) {
      altinst->newinstall = true;
    }
  }

  altinstTargetFeedbackMsg (altinst);

  return rc;
}

static int
altinstValidateName (uiwcont_t *entry, const char *label, void *udata)
{
  altinst_t     *altinst = udata;
  const char    *name;
  char          tbuff [MAXPATHLEN];
  int           rc = UIENTRY_ERROR;
  int           valflags;
  bool          val;

  if (! altinst->guienabled) {
    return UIENTRY_ERROR;
  }

  if (! altinst->uiBuilt) {
    return UIENTRY_RESET;
  }

  name = uiEntryGetValue (altinst->wcont [ALT_W_NAME]);

  /* BDJ4 is not a valid name for macos */
  /* for Linux, want to be able to type in BDJ4, for dpkg installation */
  if (isMacOS () && strcmp (name, BDJ4_NAME) == 0) {
    /* CONTEXT: alternate installer: invalid name status message */
    uiLabelSetText (altinst->wcont [ALT_W_ERROR_MSG], _("Invalid Name"));
    return rc;
  }

  /* CONTEXT: alternate installer: name (for shortcut) */
  valflags = VAL_NOT_EMPTY | VAL_NO_SLASHES;
  if (isWindows ()) {
    valflags |= VAL_NO_WINCHARS;
  }
  val = validate (tbuff, sizeof (tbuff), label, name, valflags);
  if (val == false) {
    uiLabelSetText (altinst->wcont [ALT_W_ERROR_MSG], tbuff);
    return rc;
  }

  uiLabelSetText (altinst->wcont [ALT_W_ERROR_MSG], "");
  dataFree (altinst->name);
  altinst->name = mdstrdup (name);
  rc = UIENTRY_OK;
  altinstBuildTarget (altinst, tbuff, sizeof (tbuff), altinst->name);
  altinstSetTargetEntry (altinst, tbuff);
  if (isMacOS ()) {
    /* on macos, the field is disabled, so the validation must be forced */
    rc = altinstValidateTarget (altinst->wcont [ALT_W_TARGET], "", altinst);
  }

  if (rc == UIENTRY_ERROR) {
    /* duplicate msg as above */
    uiLabelSetText (altinst->wcont [ALT_W_ERROR_MSG], _("Invalid Name"));
  }

  return rc;
}


static void
altinstTargetFeedbackMsg (altinst_t *altinst)
{
  char          tbuff [MAXPATHLEN];

  if (! altinst->guienabled) {
    return;
  }

  /* one of newinstall or updateinstall must be set */
  /* reinstall matches the re-install checkbox */
  /* if targetexists is true, and updateinstall is false, the target */
  /* folder is an existing folder, but not a bdj4 installation */

  if (altinst->updateinstall && altinst->reinstall) {
    /* CONTEXT: alternate installation: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("Re-install %s.  Overwrites existing configuration."), BDJ4_NAME);
    uiLabelSetText (altinst->wcont [ALT_W_FEEDBACK_MSG], tbuff);
  }
  if (altinst->updateinstall && ! altinst->reinstall) {
    /* CONTEXT: alternate installation: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("Updating existing %s installation."), BDJ4_NAME);
    uiLabelSetText (altinst->wcont [ALT_W_FEEDBACK_MSG], tbuff);
  }
  if (altinst->newinstall) {
    /* CONTEXT: alternate installation: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("New %s installation."), BDJ4_NAME);
    uiLabelSetText (altinst->wcont [ALT_W_FEEDBACK_MSG], tbuff);
  }
  if (altinst->targetexists && ! altinst->updateinstall) {
    /* CONTEXT: alternate installation: the selected folder exists and is not a BDJ4 installation */
    uiLabelSetText (altinst->wcont [ALT_W_FEEDBACK_MSG], _("Error: Folder already exists."));
  }
  if (! altinst->targetexists && ! altinst->newinstall && ! altinst->updateinstall) {
    /* CONTEXT: alternate installation: the selected folder is not valid */
    uiLabelSetText (altinst->wcont [ALT_W_FEEDBACK_MSG], _("Invalid Folder"));
  }

  return;
}

static bool
altinstTargetDirDialog (void *udata)
{
  altinst_t *altinst = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;

  selectdata = uiSelectInit (altinst->wcont [ALT_W_WINDOW],
      /* CONTEXT: alternate installation: dialog title for selecting location */
      _("Install Location"),
      uiEntryGetValue (altinst->wcont [ALT_W_TARGET]), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    char        tbuff [MAXPATHLEN];

    strlcpy (tbuff, fn, sizeof (tbuff));
    instutilAppendNameToTarget (tbuff, sizeof (tbuff), NULL, false);
    /* validation gets called again upon set */
    altinstSetTargetEntry (altinst, tbuff);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected target loc: %s", altinst->target);
    mdfree (fn);
  }
  uiSelectFree (selectdata);
  return UICB_CONT;
}

static bool
altinstExitCallback (void *udata)
{
  altinst_t   *altinst = udata;

  altinst->instState = ALT_EXIT;
  return UICB_CONT;
}

static bool
altinstSetupCallback (void *udata)
{
  altinst_t *altinst = udata;

  if (altinst->instState == ALT_WAIT_USER) {
    altinst->instState = ALT_INIT;
  }
  return UICB_CONT;
}

static void
altinstSetPaths (altinst_t *altinst)
{
  strlcpy (altinst->datatopdir, altinst->target, sizeof (altinst->datatopdir));
  if (isMacOS ()) {
    snprintf (altinst->datatopdir, sizeof (altinst->datatopdir),
        "%s%s/%s", altinst->home, MACOS_DIR_LIBDATA, altinst->name);
  }

  if (altinst->targetexists) {
    altinstLoadBdjOpt (altinst);
  }
}

static void
altinstPrepare (altinst_t *altinst)
{
  altinstValidateProcessTarget (altinst, altinst->target);
  altinstSetPaths (altinst);
  altinst->instState = ALT_WAIT_USER;
}

static void
altinstInit (altinst_t *altinst)
{
  altinstSetPaths (altinst);
  altinst->instState = ALT_SAVE_TARGET;
}

static void
altinstSaveTargetDir (altinst_t *altinst)
{
  /* CONTEXT: alternate installer: status message */
  altinstDisplayText (altinst, INST_DISP_ACTION, _("Saving install location."), false);

  altinstSetTargetEntry (altinst, altinst->target);

  altinst->instState = ALT_MAKE_TARGET;
}

static void
altinstMakeTarget (altinst_t *altinst)
{
  char            tbuff [MAXPATHLEN];
  sysversinfo_t   *versinfo;

  snprintf (altinst->rundir, sizeof (altinst->rundir), "%s%s",
      altinst->target, altinst->macospfx);
  diropMakeDir (altinst->rundir);

  diropMakeDir (altinst->datatopdir);

  *altinst->oldversion = '\0';
  snprintf (tbuff, sizeof (tbuff), "%s/VERSION.txt", altinst->rundir);
  if (fileopFileExists (tbuff)) {
    versinfo = sysvarsParseVersionFile (tbuff);
    instutilOldVersionString (versinfo, altinst->oldversion, sizeof (altinst->oldversion));
    sysvarsParseVersionFileFree (versinfo);
  }

  altinst->instState = ALT_CHDIR;
}

static void
altinstChangeDir (altinst_t *altinst)
{
  if (osChangeDir (altinst->datatopdir) < 0) {
    altinstFailWorkingDir (altinst, altinst->datatopdir, "cd");
    return;
  }

  if (altinst->newinstall || altinst->reinstall) {
    altinst->instState = ALT_CREATE_DIRS;
  } else {
    altinst->instState = ALT_FINALIZE;
  }
}

static void
altinstCreateDirs (altinst_t *altinst)
{
  if (altinst->updateinstall) {
    altinstLoadBdjOpt (altinst);
  }

  /* CONTEXT: alternate installation: status message */
  altinstDisplayText (altinst, INST_DISP_ACTION, _("Creating folder structure."), false);

  /* create the directories that are not included in the distribution */
  diropMakeDir ("data");
  /* this will create the directories necessary for the configs */
  instutilCreateDataDirectories ();
  diropMakeDir ("tmp");
  diropMakeDir ("http");
  /* there are profile specific image files */
  diropMakeDir ("img/profile00");

  altinst->instState = ALT_COPY_TEMPLATES;
}

static void
altinstCopyTemplates (altinst_t *altinst)
{
  /* CONTEXT: alternate installation: status message */
  altinstDisplayText (altinst, INST_DISP_ACTION, _("Copying template files."), false);

  if (osChangeDir (altinst->datatopdir) < 0) {
    altinstFailWorkingDir (altinst, altinst->datatopdir, "ct");
    return;
  }

  instutilCopyTemplates ();
  instutilCopyHttpFiles ();
  templateImageCopy (NULL);

  altinst->instState = ALT_SETUP;
}

static void
altinstSetup (altinst_t *altinst)
{
  char    buff [MAXPATHLEN];
  char    tbuff [MAXPATHLEN];
  char    tfn [MAXPATHLEN];
  FILE    *fh;
  char    str [40];
  int     altcount = 1;

  /* CONTEXT: alternate installation: status message */
  altinstDisplayText (altinst, INST_DISP_ACTION, _("Initial Setup."), false);

  /* the altcount.txt lives in the configuration dir */
  if (altinst->firstinstall) {
    FILE    *fh;

    diropMakeDir (sysvarsGetStr (SV_DIR_CONFIG));
    fh = fileopOpen (sysvarsGetStr (SV_FILE_ALTCOUNT), "w");
    if (fh != NULL) {
      fprintf (fh, "%d\n", altcount);
      mdextfclose (fh);
      fclose (fh);
    }

    /* create the new install flag file when the first installation */
    if (altinst->newinstall) {
      FILE  *fh;

      pathbldMakePath (tbuff, sizeof (tbuff),
          NEWINST_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
      fh = fileopOpen (tbuff, "w");
      mdextfclose (fh);
      fclose (fh);
    }
  }

  if (! altinst->firstinstall) {
    /* read the current altcount */
    altcount = 0;
    fh = fileopOpen (sysvarsGetStr (SV_FILE_ALTCOUNT), "r");
    if (fh != NULL) {
      (void) ! fgets (str, sizeof (str), fh);
      stringTrim (str);
      mdextfclose (fh);
      fclose (fh);
      altcount = atoi (str);
      /* update alternate count */
      ++altcount;
      altinst->altidx = altcount;
    }

    /* write the new altcount */
    fh = fileopOpen (sysvarsGetStr (SV_FILE_ALTCOUNT), "w");
    if (fh != NULL) {
      fprintf (fh, "%d\n", altinst->altidx);
      mdextfclose (fh);
      fclose (fh);
    }
  }

  /* write the altidx file out */
  pathbldMakePath (buff, sizeof (buff),
      ALT_IDX_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  fh = fileopOpen (buff, "w");
  if (fh != NULL) {
    fprintf (fh, "%d\n", altinst->altidx);
    mdextfclose (fh);
    fclose (fh);
  }

  /* write out the installation dir to the config file */
  diropMakeDir (sysvarsGetStr (SV_DIR_CONFIG));
  snprintf (tfn, sizeof (tfn), "%s/%s%s%02d%s",
      sysvarsGetStr (SV_DIR_CONFIG), ALT_INST_PATH_FN,
      sysvarsGetStr (SV_BDJ4_DEVELOPMENT), altinst->altidx, BDJ4_CONFIG_EXT);

  fh = fileopOpen (tfn, "w");
  if (fh != NULL) {
    fprintf (fh, "%s\n", altinst->target);
    mdextfclose (fh);
    fclose (fh);
  }

  /* copy the VERSION.txt file so that the old version can be tracked */
  snprintf (buff, sizeof (buff), "%s/VERSION.txt", altinst->maindir);
  snprintf (tbuff, sizeof (tbuff), "%s/VERSION.txt", altinst->rundir);
  filemanipCopy (buff, tbuff);

  /* now switch back to the run-dir */

  if (osChangeDir (altinst->rundir) < 0) {
    altinstFailWorkingDir (altinst, altinst->rundir, "setup");
    return;
  }

  diropMakeDir ("bin");

  /* create the symlink for the bdj4 executable */
  /* this is unique to an Linux alternate installation */
  /* Window and MacOS are handled by the create-launcher script */
  if (isLinux ()) {
#if _lib_symlink
    pathbldMakePath (buff, sizeof (buff),
        altinst->launchname, "", PATHBLD_MP_DIR_EXEC);
    snprintf (tbuff, sizeof (tbuff), "bin/%s", altinst->launchname);
    (void) ! symlink (buff, tbuff);
#endif
  }

  pathbldMakePath (buff, sizeof (buff),
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CACHE);
  fileopDelete (buff);
  pathbldMakePath (buff, sizeof (buff),
      VOLREG_FN, BDJ4_LOCK_EXT, PATHBLD_MP_DIR_CACHE);
  fileopDelete (buff);

  altinst->instState = ALT_CREATE_LAUNCHER;
}

static void
altinstCreateLauncher (altinst_t *altinst)
{
  const char  *name;

  /* for linux and windows, the desktop launcher can have arguments */
  /* the main bdj4 executable can be started directly */
  /* and the data-top-dir and profile number can be */
  /* supplied as arguments */
  /* on macos, a .app is created */

  if (osChangeDir (altinst->maindir) < 0) {
    altinstFailWorkingDir (altinst, altinst->maindir, "cdl");
    return;
  }

  /* CONTEXT: alternate installation: status message */
  altinstDisplayText (altinst, INST_DISP_ACTION, _("Creating shortcut."), false);
  name = altinst->name;
  if (altinst->guienabled) {
    name = uiEntryGetValue (altinst->wcont [ALT_W_NAME]);
  }

  instutilCreateLauncher (name, altinst->maindir, altinst->datatopdir, 0);

  altinst->instState = ALT_FINALIZE;
}

static void
altinstFinalize (altinst_t *altinst)
{
  uint32_t  baseport;
  char      tbuff [MAXPATHLEN];
  FILE      *fh;

  if (osChangeDir (altinst->datatopdir) < 0) {
    altinstFailWorkingDir (altinst, altinst->datatopdir, "fin");
    return;
  }

  /* the base port must always be updated in case the number of ports */
  /* used in BDJ4 has been changed. */
  /* calculate the new base port */
  baseport = (uint32_t) sysvarsGetNum (SVL_INITIAL_PORT);
  baseport += altinst->altidx * BDJOPT_MAX_PROFILES *
      (int) bdjvarsGetNum (BDJVL_NUM_PORTS);

  /* write the new base port out */
  pathbldMakePath (tbuff, sizeof (tbuff),
      BASE_PORT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  fh = fileopOpen (tbuff, "w");
  if (fh != NULL) {
    fprintf (fh, "%" PRIu32 "\n", baseport);
    mdextfclose (fh);
    fclose (fh);
  }

  if (altinst->newinstall) {
    if (! altinst->bdjoptloaded) {
      altinstLoadBdjOpt (altinst);
    }
    if (altinst->bdjoptloaded) {
      char  tbuff [MAXPATHLEN];

      instutilGetMusicDir (tbuff, sizeof (tbuff));
      bdjoptSetStr (OPT_M_DIR_MUSIC, tbuff);
      bdjoptSave ();
    }
  }

  if (altinst->verbose) {
    fprintf (stdout, "finish OK\n");
    fprintf (stdout, "bdj3-version x\n");
    if (*altinst->oldversion) {
      fprintf (stdout, "old-version %s\n", altinst->oldversion);
    } else {
      fprintf (stdout, "old-version x\n");
    }
    fprintf (stdout, "first-install %d\n", altinst->firstinstall);
    fprintf (stdout, "new-install %d\n", altinst->newinstall);
    fprintf (stdout, "re-install %d\n", altinst->reinstall);
    fprintf (stdout, "update %d\n", altinst->updateinstall);
    fprintf (stdout, "converted 0\n");
    fprintf (stdout, "readonly 0\n");
    fprintf (stdout, "alternate 1\n");
  }

  altinst->instState = ALT_UPDATE_PROCESS_INIT;
}

static void
altinstUpdateProcessInit (altinst_t *altinst)
{
  char  buff [MAXPATHLEN];

  if (osChangeDir (altinst->datatopdir) < 0) {
    altinstFailWorkingDir (altinst, altinst->datatopdir, "upi");
    return;
  }

  /* CONTEXT: alternate installation: status message */
  snprintf (buff, sizeof (buff), _("Updating %s."), BDJ4_LONG_NAME);
  altinstDisplayText (altinst, INST_DISP_ACTION, buff, false);
  altinst->instState = ALT_UPDATE_PROCESS;
}

static void
altinstUpdateProcess (altinst_t *altinst)
{
  char  tbuff [MAXPATHLEN];
  int   targc = 0;
  const char  *targv [10];

  snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4%s",
      altinst->maindir, sysvarsGetStr (SV_OS_EXEC_EXT));
  targv [targc++] = tbuff;
  targv [targc++] = "--bdj4updater";

  /* only need to run the 'newinstall' update process when the template */
  /* files have been copied */
  if (altinst->newinstall || altinst->reinstall) {
    targv [targc++] = "--newinstall";
  }
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);

  altinst->instState = ALT_FINISH;
}

/* support routines */

static void
altinstCleanup (altinst_t *altinst)
{
  if (altinst != NULL) {
    for (int i = 0; i < ALT_CB_MAX; ++i) {
      callbackFree (altinst->callbacks [i]);
    }
    if (altinst->guienabled) {
      for (int i = 0; i < ALT_W_MAX; ++i) {
        uiwcontFree (altinst->wcont [i]);
      }
    }
    dataFree (altinst->target);
    dataFree (altinst->name);
  }
}

static void
altinstDisplayText (altinst_t *altinst, char *pfx, char *txt, bool bold)
{
  if (altinst->guienabled) {
    if (bold) {
      uiTextBoxAppendBoldStr (altinst->wcont [ALT_W_STATUS_DISP], pfx);
      uiTextBoxAppendBoldStr (altinst->wcont [ALT_W_STATUS_DISP], txt);
      uiTextBoxAppendBoldStr (altinst->wcont [ALT_W_STATUS_DISP], "\n");
    } else {
      uiTextBoxAppendStr (altinst->wcont [ALT_W_STATUS_DISP], pfx);
      uiTextBoxAppendStr (altinst->wcont [ALT_W_STATUS_DISP], txt);
      uiTextBoxAppendStr (altinst->wcont [ALT_W_STATUS_DISP], "\n");
    }
    altinst->scrolltoend = true;
  } else {
    if (! altinst->quiet) {
      fprintf (stdout, "%s%s\n", pfx, txt);
      fflush (stdout);
    }
  }
}

static void
altinstFailWorkingDir (altinst_t *altinst, const char *dir, const char *tag)
{
  fprintf (stderr, "Unable to set working dir: %s %s\n", tag, dir);
  /* CONTEXT: alternate installation: failure message */
  altinstDisplayText (altinst, "", _("Error: Unable to set working folder."), false);
  /* CONTEXT: alternate installation: status message */
  altinstDisplayText (altinst, INST_DISP_ERROR, _("Process aborted."), false);
  altinst->instState = ALT_WAIT_USER;
}

static void
altinstSetTargetDir (altinst_t *altinst, const char *fn)
{
  char        *tmp;
  pathinfo_t  *pi;

  tmp = mdstrdup (fn);
  dataFree (altinst->target);
  altinst->target = tmp;
  pathNormalizePath (altinst->target, strlen (altinst->target));

  pi = pathInfo (altinst->target);
  dataFree (altinst->basedir);
  altinst->basedir = mdmalloc (pi->dlen + 1);
  strlcpy (altinst->basedir, pi->dirname, pi->dlen + 1);
  altinst->basedir [pi->dlen] = '\0';
  pathInfoFree (pi);
}

static void
altinstLoadBdjOpt (altinst_t *altinst)
{
  char        cwd [MAXPATHLEN];

  if (altinst->bdjoptloaded) {
    bdjoptCleanup ();
    altinst->bdjoptloaded = false;
  }

  osGetCurrentDir (cwd, sizeof (cwd));
  if (osChangeDir (altinst->datatopdir) < 0) {
    return;
  }

  bdjoptInit ();
  altinst->bdjoptloaded = true;

  if (osChangeDir (cwd) < 0) {
    fprintf (stderr, "ERR: Unable to chdir to %s\n", cwd);
  }
}

static void
altinstBuildTarget (altinst_t *altinst, char *buff, size_t sz, const char *nm)
{
  strlcpy (buff, altinst->basedir, sz);
  if (isMacOS ()) {
    snprintf (buff, sz, "%s/Applications", altinst->basedir);
  }
  instutilAppendNameToTarget (buff, sz, nm, false);
}

static void
altinstSigHandler (int sig)
{
  return;
}

static void
altinstSetTargetEntry (altinst_t *altinst, const char *fn)
{
  char    tbuff [MAXPATHLEN];

  strlcpy (tbuff, fn, sizeof (tbuff));
  pathDisplayPath (tbuff, sizeof (tbuff));
  uiEntrySetValue (altinst->wcont [ALT_W_TARGET], tbuff);
}
