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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <signal.h>

#include "bdj4.h"
#include "bdj4arg.h"
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
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "templateutil.h"
#include "tmutil.h"
#include "ui.h"

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
  ALT_CREATE_SHORTCUT,
  ALT_FINALIZE,
  ALT_UPDATE_PROCESS_INIT,
  ALT_UPDATE_PROCESS,
  ALT_REGISTER_INIT,
  ALT_REGISTER,
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
  ALT_BUTTON_TARGET_DIR,
  ALT_BUTTON_EXIT,
  ALT_BUTTON_START,
  ALT_BUTTON_MAX,
};

enum {
  ALT_TARGET,
};

enum {
  ALT_W_WINDOW,
  ALT_W_REINST,
  ALT_W_FEEDBACK_MSG,
  ALT_W_MAX,
};

typedef struct {
  altinststate_t  instState;
  altinststate_t  lastInstState;            // debugging
  callback_t      *callbacks [ALT_CB_MAX];
  uibutton_t      *buttons [ALT_BUTTON_MAX];
  char            oldversion [MAXPATHLEN];
  char            *target;
  char            *macospfx;
  char            *maindir;
  char            *hostname;
  char            *home;
  char            *name;
  /* conversion */
  uiwcont_t       *wcont [ALT_W_MAX];
  uientry_t       *targetEntry;
  uientry_t       *nameEntry;
  uitextbox_t     *disptb;
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
static int  altinstValidateTarget (uientry_t *entry, void *udata);
static int  altinstValidateProcessTarget (altinst_t *altinst, const char *dir);
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
static void altinstCreateShortcut (altinst_t *altinst);
static void altinstUpdateProcessInit (altinst_t *altinst);
static void altinstUpdateProcess (altinst_t *altinst);
static void altinstFinalize (altinst_t *altinst);
static void altinstRegisterInit (altinst_t *altinst);
static void altinstRegister (altinst_t *altinst);

static void altinstCleanup (altinst_t *altinst);
static void altinstDisplayText (altinst_t *altinst, char *pfx, char *txt, bool bold);
static void altinstFailWorkingDir (altinst_t *altinst, const char *dir);
static void altinstSetTargetDir (altinst_t *altinst, const char *fn);
static void altinstLoadBdjOpt (altinst_t *altinst);

int
main (int argc, char *argv[])
{
  altinst_t     altinst;
  char          buff [MAXPATHLEN];
  char          *uifont;
  int           c = 0;
  int           option_index = 0;
  FILE          *fh;
  bdj4arg_t   *bdj4arg;
  const char  *targ;

  static struct option bdj_options [] = {
    { "bdj4altinst",no_argument,       NULL,   0 },
    { "datatopdir", required_argument,  NULL,   'D' },
    { "locale",     required_argument,  NULL,   'L' },
    { "name",       required_argument,  NULL,   'n' },
    { "reinstall",  no_argument,        NULL,   'r' },
    { "targetdir",  required_argument,  NULL,   't' },
    { "testregistration", no_argument,  NULL,   'T' },
    { "unattended", no_argument,        NULL,   'U' },
    /* generic args */
    { "quiet"  ,    no_argument,        NULL,   'Q' },
    { "verbose",    no_argument,        NULL,   'V' },
    /* bdj4 launcher args */
    { "bdj4",       no_argument,        NULL,   0 },
    { "debug",      required_argument,  NULL,   'd' },
    { "debugself",  no_argument,        NULL,   0 },
    { "nodetach",   no_argument,        NULL,   0 },
    { "scale",      required_argument,  NULL,   0 },
    { "theme",      required_argument,  NULL,   0 },
    { "wait",       no_argument,        NULL,   0 },
    { "origcwd",    required_argument,  NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("alt");
#endif

  bdj4arg = bdj4argInit (argc, argv);

  buff [0] = '\0';

  altinst.instState = ALT_PRE_INIT;
  altinst.lastInstState = ALT_PRE_INIT;
  altinst.target = mdstrdup ("");
  altinst.macospfx = "";
  strcpy (altinst.oldversion, "");
  altinst.maindir = NULL;
  altinst.home = NULL;
  altinst.name = mdstrdup ("BDJ4 B");
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
  for (int i = 0; i < ALT_W_MAX; ++i) {
    altinst.wcont [i] = NULL;
  }
  for (int i = 0; i < ALT_BUTTON_MAX; ++i) {
    altinst.buttons [i] = NULL;
  }
  for (int i = 0; i < ALT_CB_MAX; ++i) {
    altinst.callbacks [i] = NULL;
  }
  altinst.targetEntry = NULL;
  altinst.nameEntry = NULL;

  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ);
  bdjvarsInit ();
  localeInit ();

  strlcpy (buff, sysvarsGetStr (SV_HOME), sizeof (buff));
  if (isMacOS ()) {
    altinst.macospfx = MACOS_PREFIX;
    snprintf (buff, sizeof (buff), "%s/Applications", sysvarsGetStr (SV_HOME));
  }
  instutilAppendNameToTarget (buff, sizeof (buff), false);

  fh = fileopOpen (sysvarsGetStr (SV_FILE_ALT_INST_PATH), "r");
  if (fh != NULL) {
    (void) ! fgets (buff, sizeof (buff), fh);
    stringTrim (buff);
    mdextfclose (fh);
    fclose (fh);
  }

  altinstSetTargetDir (&altinst, buff);

  /* the altcount.txt lives in the configuration dir */
  if (! fileopFileExists (sysvarsGetStr (SV_FILE_ALTCOUNT))) {
    altinst.firstinstall = true;
  }

  if (altinst.firstinstall) {
    dataFree (altinst.name);
    altinst.name = mdstrdup (BDJ4_NAME);
  }

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "CUrVQt:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'U': {
        altinst.unattended = true;
        altinst.guienabled = false;
#if _define_SIGCHLD
        osDefaultSignal (SIGCHLD);
#endif
        break;
      }
      case 'r': {
        altinst.reinstall = true;
        break;
      }
      case 'L': {
        if (optarg != NULL) {
          sysvarsSetStr (SV_LOCALE, optarg);
          snprintf (buff, sizeof (buff), "%.2s", optarg);
          sysvarsSetStr (SV_LOCALE_SHORT, buff);
          sysvarsSetNum (SVL_LOCALE_SET, 1);
          altinst.localespecified = true;
          localeSetup ();
        }
        break;
      }
      case 'V': {
        altinst.verbose = true;
        break;
      }
      case 'Q': {
        altinst.quiet = true;
        break;
      }
      case 't': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          altinstSetTargetDir (&altinst, optarg);
        }
        break;
      }
      case 'D': {
        break;
      }
      case 'T': {
        altinst.testregistration = true;
        break;
      }
      default: {
        break;
      }
    }
  }

  if (altinst.guienabled) {
    altinst.targetEntry = uiEntryInit (80, MAXPATHLEN);
    altinst.nameEntry = uiEntryInit (30, 30);
  }

  altinst.maindir = sysvarsGetStr (SV_BDJ4_DIR_MAIN);
  altinst.home = sysvarsGetStr (SV_HOME);
  altinst.hostname = sysvarsGetStr (SV_HOSTNAME);

  if (osChangeDir (altinst.maindir)) {
    fprintf (stderr, "ERR: Unable to chdir to %s\n", altinst.maindir);
  }

  if (altinst.guienabled) {
    uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));

    uifont = sysvarsGetStr (SV_FONT_DEFAULT);
    if (uifont == NULL || ! *uifont) {
      uifont = "Arial Regular 11";
      if (isMacOS ()) {
        uifont = "Arial Regular 17";
      }
    }
    uiSetUICSS (uifont, INST_HL_COLOR, NULL);

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
  bdjvarsCleanup ();
  localeCleanup ();
  logEnd ();
  bdj4argCleanup (bdj4arg);
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
  uibutton_t    *uibutton;
  uiwcont_t     *uiwidgetp;
  char          tbuff [100];
  char          imgbuff [MAXPATHLEN];

  uiLabelAddClass (INST_HL_CLASS, INST_HL_COLOR);
  uiSeparatorAddClass (INST_SEP_CLASS, INST_SEP_COLOR);

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
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiWindowPackInWindow (altinst->wcont [ALT_W_WINDOW], vbox);

  uiwidgetp = uiCreateLabel (
      /* CONTEXT: alternate installation: ask for installation folder */
      _("Enter the destination folder where BDJ4 will be installed."));
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* begin line : target entry */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiEntryCreate (altinst->targetEntry);
  uiEntrySetValue (altinst->targetEntry, altinst->target);
  uiwidgetp = uiEntryGetWidgetContainer (altinst->targetEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiEntrySetValidate (altinst->targetEntry,
      altinstValidateTarget, altinst, UIENTRY_DELAYED);

  altinst->callbacks [ALT_CB_TARGET_DIR] = callbackInit (
      altinstTargetDirDialog, altinst, NULL);
  uibutton = uiCreateButton (
      altinst->callbacks [ALT_CB_TARGET_DIR],
      "", NULL);
  altinst->buttons [ALT_BUTTON_TARGET_DIR] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);

  /* begin line */
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
  uiWidgetSetClass (altinst->wcont [ALT_W_FEEDBACK_MSG], INST_HL_CLASS);
  uiBoxPackStart (hbox, altinst->wcont [ALT_W_FEEDBACK_MSG]);

  uiwcontFree (hbox);

  /* begin line : alternate name */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: alternate installation: name (for shortcut) */
      _("Name"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiEntryCreate (altinst->nameEntry);
  uiEntrySetValue (altinst->nameEntry, altinst->name);
  uiwidgetp = uiEntryGetWidgetContainer (altinst->nameEntry);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);

  /* begin line */
  /* button box */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uibutton = uiCreateButton (
      altinst->callbacks [ALT_CB_EXIT],
      /* CONTEXT: alternate installation: exits the alternate installer */
      _("Exit"), NULL);
  altinst->buttons [ALT_BUTTON_EXIT] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackEnd (hbox, uiwidgetp);

  altinst->callbacks [ALT_CB_START] = callbackInit (
      altinstSetupCallback, altinst, NULL);
  uibutton = uiCreateButton (
      altinst->callbacks [ALT_CB_START],
      /* CONTEXT: alternate installation: install BDJ4 in the alternate location */
      _("Install"), NULL);
  altinst->buttons [ALT_BUTTON_START] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackEnd (hbox, uiwidgetp);

  altinst->disptb = uiTextBoxCreate (200, NULL);
  uiTextBoxSetReadonly (altinst->disptb);
  uiTextBoxHorizExpand (altinst->disptb);
  uiTextBoxVertExpand (altinst->disptb);
  uiBoxPackStartExpand (vbox, uiTextBoxGetScrolledWindow (altinst->disptb));

  uiWidgetShowAll (altinst->wcont [ALT_W_WINDOW]);
  altinst->uiBuilt = true;

  uiwcontFree (vbox);
  uiwcontFree (hbox);
}

static int
altinstMainLoop (void *udata)
{
  altinst_t *altinst = udata;

  if (altinst->guienabled) {
    uiUIProcessEvents ();

    uiEntryValidate (altinst->targetEntry, false);

    if (altinst->scrolltoend) {
      uiTextBoxScrollToEnd (altinst->disptb);
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

      logStart ("bdj4altinst", "alt",
          LOG_IMPORTANT | LOG_BASIC | LOG_INFO | LOG_REDIR_INST);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "=== alternate setup started");
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
    case ALT_CREATE_SHORTCUT: {
      altinstCreateShortcut (altinst);
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
    case ALT_REGISTER_INIT: {
      altinstRegisterInit (altinst);
      break;
    }
    case ALT_REGISTER: {
      altinstRegister (altinst);
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
altinstValidateTarget (uientry_t *entry, void *udata)
{
  altinst_t   *altinst = udata;
  const char    *dir;
  char          tbuff [MAXPATHLEN];
  int           rc = UIENTRY_ERROR;

  if (! altinst->guienabled) {
    return UIENTRY_ERROR;
  }

  if (! altinst->uiBuilt) {
    return UIENTRY_RESET;
  }

  dir = uiEntryGetValue (altinst->targetEntry);
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
        instutilAppendNameToTarget (tbuff, sizeof (tbuff), true);
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
      snprintf (tmp, sizeof (tmp), "%.*s", (int) pi->dlen, pi->dirname);
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

static void
altinstTargetFeedbackMsg (altinst_t *altinst)
{
  char          tbuff [MAXPATHLEN];

  /* one of newinstall or updateinstall must be set */
  /* reinstall matches the re-install checkbox */
  /* if targetexists is true, and updateinstall is false, the target */
  /* folder is an existing folder, but not a bdj4 installation */

  if (altinst->updateinstall && altinst->reinstall) {
    /* CONTEXT: alternate installation: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("Re-install %s."), BDJ4_NAME);
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

  selectdata = uiDialogCreateSelect (altinst->wcont [ALT_W_WINDOW],
      /* CONTEXT: alternate installation: dialog title for selecting location */
      _("Install Location"),
      uiEntryGetValue (altinst->targetEntry), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    char        tbuff [MAXPATHLEN];

    strlcpy (tbuff, fn, sizeof (tbuff));
    instutilAppendNameToTarget (tbuff, sizeof (tbuff), false);
    /* validation gets called again upon set */
    uiEntrySetValue (altinst->targetEntry, tbuff);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected target loc: %s", altinst->target);
    mdfree (fn);
  }
  mdfree (selectdata);
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
  FILE        *fh;

  /* CONTEXT: alternate installer: status message */
  altinstDisplayText (altinst, INST_DISP_ACTION, _("Saving install location."), false);

  uiEntrySetValue (altinst->targetEntry, altinst->target);

  diropMakeDir (sysvarsGetStr (SV_DIR_CONFIG));
  fh = fileopOpen (sysvarsGetStr (SV_FILE_ALT_INST_PATH), "w");
  if (fh != NULL) {
    fprintf (fh, "%s\n", altinst->target);
    mdextfclose (fh);
    fclose (fh);
  }

  altinst->instState = ALT_MAKE_TARGET;
}

static void
altinstMakeTarget (altinst_t *altinst)
{
  char            tbuff [MAXPATHLEN];
  sysversinfo_t   *versinfo;

  diropMakeDir (altinst->target);

  *altinst->oldversion = '\0';
  snprintf (tbuff, sizeof (tbuff), "%s/VERSION.txt", altinst->target);
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
  if (osChangeDir (altinst->target)) {
    altinstFailWorkingDir (altinst, altinst->target);
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
  bdjoptCreateDirectories ();
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

  if (osChangeDir (altinst->target)) {
    altinstFailWorkingDir (altinst, altinst->target);
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
  FILE    *fh;
  char    str [40];
  int     altcount = 0;
  int     baseport;

  /* CONTEXT: alternate installation: status message */
  altinstDisplayText (altinst, INST_DISP_ACTION, _("Initial Setup."), false);

  /* the altcount.txt lives in the configuration dir */
  if (altinst->firstinstall) {
    FILE    *fh;

    diropMakeDir (sysvarsGetStr (SV_DIR_CONFIG));
    fh = fileopOpen (sysvarsGetStr (SV_FILE_ALTCOUNT), "w");
    if (fh != NULL) {
      fputs ("0\n", fh);
      mdextfclose (fh);
      fclose (fh);
    }

    /* create the new install flag file when the first installation */
    if (altinst->newinstall) {
      FILE  *fh;
      char  tbuff [MAXPATHLEN];

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
    }

    /* write the new altcount */
    fh = fileopOpen (sysvarsGetStr (SV_FILE_ALTCOUNT), "w");
    if (fh != NULL) {
      snprintf (str, sizeof (str), "%d\n", altcount);
      fputs (str, fh);
      mdextfclose (fh);
      fclose (fh);
    }
  }

  /* calculate the new base port */
  baseport = sysvarsGetNum (SVL_INITIAL_PORT);
  baseport += altcount * BDJOPT_MAX_PROFILES * (int) bdjvarsGetNum (BDJVL_NUM_PORTS);

  /* write the new base port out */
  snprintf (str, sizeof (str), "%d\n", baseport);
  pathbldMakePath (buff, sizeof (buff),
      BASE_PORT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  fh = fileopOpen (buff, "w");
  if (fh != NULL) {
    fputs (str, fh);
    mdextfclose (fh);
    fclose (fh);
  }

  /* write the alt-idx file out */
  snprintf (str, sizeof (str), "%d\n", altcount);
  pathbldMakePath (buff, sizeof (buff),
      ALT_IDX_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  fh = fileopOpen (buff, "w");
  if (fh != NULL) {
    fputs (str, fh);
    mdextfclose (fh);
    fclose (fh);
  }

  /* copy the VERSION.txt file so that the old version can be tracked */
  snprintf (buff, sizeof (buff), "%s/VERSION.txt", altinst->maindir);
  snprintf (tbuff, sizeof (tbuff), "%s/VERSION.txt", altinst->target);
  filemanipCopy (buff, tbuff);

// ### FIX : check is dir-exec even set?
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

  pathbldMakePath (buff, sizeof (buff),
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CACHE);
  fileopDelete (buff);
  pathbldMakePath (buff, sizeof (buff),
      VOLREG_FN, BDJ4_LOCK_EXT, PATHBLD_MP_DIR_CACHE);
  fileopDelete (buff);

  altinst->instState = ALT_CREATE_SHORTCUT;
}

static void
altinstCreateShortcut (altinst_t *altinst)
{
  const char  *name;

  if (osChangeDir (altinst->maindir)) {
    altinstFailWorkingDir (altinst, altinst->maindir);
    return;
  }

  /* CONTEXT: alternate installation: status message */
  altinstDisplayText (altinst, INST_DISP_ACTION, _("Creating shortcut."), false);
  name = altinst->name;
  if (altinst->guienabled) {
    name = uiEntryGetValue (altinst->nameEntry);
  }
  instutilCreateShortcut (name, altinst->maindir, altinst->target, 0);

  altinst->instState = ALT_FINALIZE;
}

static void
altinstFinalize (altinst_t *altinst)
{
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
  }

  if (altinst->firstinstall) {
    altinst->instState = ALT_UPDATE_PROCESS_INIT;
  } else {
    altinst->instState = ALT_FINISH;
  }
}

static void
altinstUpdateProcessInit (altinst_t *altinst)
{
  char  buff [MAXPATHLEN];

  if (osChangeDir (altinst->target)) {
    altinstFailWorkingDir (altinst, altinst->target);
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

  altinst->instState = ALT_REGISTER_INIT;
}

static void
altinstRegisterInit (altinst_t *altinst)
{
  char    tbuff [200];

  if (strcmp (sysvarsGetStr (SV_USER), "bll") == 0 &&
      ! altinst->testregistration) {
    /* no need to register */
    snprintf (tbuff, sizeof (tbuff), "Registration Skipped.");
    altinstDisplayText (altinst, INST_DISP_ACTION, tbuff, false);
    altinst->instState = ALT_FINISH;
  } else {
    /* CONTEXT: alternate installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Registering %s."), BDJ4_NAME);
    altinstDisplayText (altinst, INST_DISP_ACTION, tbuff, false);
    altinst->instState = ALT_REGISTER;
  }
}

static void
altinstRegister (altinst_t *altinst)
{
  char          tbuff [500];

  snprintf (tbuff, sizeof (tbuff),
      "&bdj3version=%s&oldversion=%s"
      "&new=%d&reinstall=%d&update=%d&convert=%d&readonly=%d",
      "",
      altinst->oldversion,
      altinst->newinstall,
      altinst->reinstall,
      altinst->updateinstall,
      false,
      false
      );
  instutilRegister (tbuff);
  altinst->instState = ALT_FINISH;
}

/* support routines */

static void
altinstCleanup (altinst_t *altinst)
{
  if (altinst->bdjoptloaded) {
    bdjoptCleanup ();
  }

  if (altinst != NULL) {
    for (int i = 0; i < ALT_CB_MAX; ++i) {
      callbackFree (altinst->callbacks [i]);
    }
    if (altinst->guienabled) {
      for (int i = 0; i < ALT_W_MAX; ++i) {
        uiwcontFree (altinst->wcont [i]);
      }
      for (int i = 0; i < ALT_BUTTON_MAX; ++i) {
        uiButtonFree (altinst->buttons [i]);
      }
      uiEntryFree (altinst->targetEntry);
      uiEntryFree (altinst->nameEntry);
      uiTextBoxFree (altinst->disptb);
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
      uiTextBoxAppendBoldStr (altinst->disptb, pfx);
      uiTextBoxAppendBoldStr (altinst->disptb, txt);
      uiTextBoxAppendBoldStr (altinst->disptb, "\n");
    } else {
      uiTextBoxAppendStr (altinst->disptb, pfx);
      uiTextBoxAppendStr (altinst->disptb, txt);
      uiTextBoxAppendStr (altinst->disptb, "\n");
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
altinstFailWorkingDir (altinst_t *altinst, const char *dir)
{
  fprintf (stderr, "Unable to set working dir: %s\n", dir);
  /* CONTEXT: alternate installation: failure message */
  altinstDisplayText (altinst, "", _("Error: Unable to set working folder."), false);
  /* CONTEXT: alternate installation: status message */
  altinstDisplayText (altinst, INST_DISP_ERROR, _("Process aborted."), false);
  altinst->instState = ALT_WAIT_USER;
}

static void
altinstSetTargetDir (altinst_t *altinst, const char *fn)
{
  char      *tmp;

  tmp = mdstrdup (fn);
  dataFree (altinst->target);
  altinst->target = tmp;
  pathNormalizePath (altinst->target, strlen (altinst->target));
}

static void
altinstLoadBdjOpt (altinst_t *altinst)
{
  char        cwd [MAXPATHLEN];

  if (altinst->bdjoptloaded) {
    bdjoptCleanup ();
    altinst->bdjoptloaded = false;
  }

  if (altinst->newinstall) {
    return;
  }

  osGetCurrentDir (cwd, sizeof (cwd));
  if (osChangeDir (altinst->target)) {
    return;
  }

  bdjoptInit ();
  altinst->bdjoptloaded = true;

  if (osChangeDir (cwd)) {
    fprintf (stderr, "ERR: Unable to chdir to %s\n", cwd);
  }
}

