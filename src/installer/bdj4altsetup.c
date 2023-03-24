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
#include "callback.h"
#include "datafile.h"
#include "dirlist.h"
#include "dirop.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "instutil.h"
#include "localeutil.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "ossignal.h"
#include "osutils.h"
#include "osuiutils.h"
#include "pathbld.h"
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

typedef struct {
  altsetupstate_t instState;
  callback_t      *callbacks [ALT_CB_MAX];
  uibutton_t      *buttons [ALT_BUTTON_MAX];
  char            *target;
  char            *maindir;
  char            *hostname;
  char            *home;
  char            dlfname [MAXPATHLEN];
  /* conversion */
  uiwcont_t       *window;
  uientry_t       *targetEntry;
  uientry_t       *nameEntry;
  uiwcont_t       *reinstWidget;
  uiwcont_t       feedbackMsg;
  uitextbox_t     *disptb;
  /* flags */
  bool            uiBuilt : 1;
  bool            scrolltoend : 1;
  bool            newinstall : 1;
  bool            reinstall : 1;
  bool            guienabled : 1;
  bool            quiet : 1;
  bool            verbose : 1;
  bool            unattended : 1;
} altsetup_t;

#define INST_HL_COLOR "#b16400"
#define INST_HL_CLASS "insthl"
#define INST_SEP_COLOR "#733000"
#define INST_SEP_CLASS "instsep"

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
static void altsetupFailWorkingDir (altsetup_t *altsetup, const char *dir);
static void altsetupSetTargetDir (altsetup_t *altsetup, const char *fn);

int
main (int argc, char *argv[])
{
  altsetup_t    altsetup;
  char          buff [MAXPATHLEN];
  char          *uifont;
  int           c = 0;
  int           option_index = 0;

  static struct option bdj_options [] = {
    { "bdj4altsetup",no_argument,       NULL,   0 },
    { "targetdir",  required_argument,  NULL,   't' },
    { "unattended", no_argument,        NULL,   'U' },
    /* generic args */
    { "cli",        no_argument,        NULL,   'C' },
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
    { NULL,         0,                  NULL,   0 }
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("alt");
#endif

  buff [0] = '\0';

  altsetup.window = NULL;
  altsetup.instState = ALT_PRE_INIT;
  altsetup.target = mdstrdup ("");
  altsetup.uiBuilt = false;
  altsetup.scrolltoend = false;
  altsetup.maindir = NULL;
  altsetup.home = NULL;
  altsetup.hostname = NULL;
  altsetup.newinstall = false;
  altsetup.reinstall = false;
  altsetup.guienabled = true;
  altsetup.quiet = false;
  altsetup.verbose = false;
  altsetup.unattended = false;
  altsetup.reinstWidget = NULL;
  uiwcontInit (&altsetup.feedbackMsg);
  for (int i = 0; i < ALT_BUTTON_MAX; ++i) {
    altsetup.buttons [i] = NULL;
  }
  for (int i = 0; i < ALT_CB_MAX; ++i) {
    altsetup.callbacks [i] = NULL;
  }

  while ((c = getopt_long_only (argc, argv, "CUrVQt:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'U': {
        altsetup.unattended = true;
        altsetup.guienabled = false;
        break;
      }
      case 'r': {
        altsetup.reinstall = true;
        break;
      }
      case 'V': {
        altsetup.verbose = true;
        break;
      }
      case 'Q': {
        altsetup.quiet = true;
        break;
      }
      case 't': {
        altsetupSetTargetDir (&altsetup, optarg);
        break;
      }
      default: {
        break;
      }
    }
  }

  if (altsetup.guienabled) {
    altsetup.targetEntry = uiEntryInit (80, MAXPATHLEN);
    altsetup.nameEntry = uiEntryInit (30, 30);
  }

  sysvarsInit (argv[0]);
  bdjvarsInit ();
  localeInit ();

  altsetup.maindir = sysvarsGetStr (SV_BDJ4_DIR_MAIN);
  altsetup.home = sysvarsGetStr (SV_HOME);
  altsetup.hostname = sysvarsGetStr (SV_HOSTNAME);

  if (altsetup.guienabled) {
    uiUIInitialize ();

    uifont = sysvarsGetStr (SV_FONT_DEFAULT);
    if (uifont == NULL || ! *uifont) {
      uifont = "Arial Regular 11";
      if (isMacOS ()) {
        uifont = "Arial Regular 17";
      }
    }
    uiSetUICSS (uifont, INST_HL_COLOR, NULL);

    altsetupBuildUI (&altsetup);
    osuiFinalize ();
  }

  while (altsetup.instState != ALT_EXIT) {
    altsetupMainLoop (&altsetup);
    mssleep (10);
  }

  if (altsetup.guienabled) {
    /* process any final events */
    uiUIProcessEvents ();
    uiCloseWindow (altsetup.window);
    uiCleanup ();
  }

  altsetupCleanup (&altsetup);
  bdjvarsCleanup ();
  localeCleanup ();
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

static void
altsetupBuildUI (altsetup_t *altsetup)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     uiwidget;
  uibutton_t    *uibutton;
  uiwcont_t     *uiwidgetp;
  char          tbuff [100];
  char          imgbuff [MAXPATHLEN];

  uiLabelAddClass (INST_HL_CLASS, INST_HL_COLOR);
  uiSeparatorAddClass (INST_SEP_CLASS, INST_SEP_COLOR);

  uiwcontInit (&uiwidget);

  strlcpy (imgbuff, "img/bdj4_icon_inst.png", sizeof (imgbuff));
  osuiSetIcon (imgbuff);

  strlcpy (imgbuff, "img/bdj4_icon_inst.svg", sizeof (imgbuff));
  /* CONTEXT: set up alternate: window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Set Up Alternate"), BDJ4_NAME);
  altsetup->callbacks [ALT_CB_EXIT] = callbackInit (
      altsetupExitCallback, altsetup, NULL);
  altsetup->window = uiCreateMainWindow (
      altsetup->callbacks [ALT_CB_EXIT], tbuff, imgbuff);
  uiWindowSetDefaultSize (altsetup->window, 1000, 600);

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiBoxPackInWindow (altsetup->window, vbox);

  uiCreateLabelOld (&uiwidget,
      /* CONTEXT: set up alternate: ask for alternate folder */
      _("Enter the alternate folder."));
  uiBoxPackStart (vbox, &uiwidget);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiEntryCreate (altsetup->targetEntry);
  uiEntrySetValue (altsetup->targetEntry, altsetup->target);
  uiwidgetp = uiEntryGetWidgetContainer (altsetup->targetEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiEntrySetValidate (altsetup->targetEntry,
      altsetupValidateTarget, altsetup, UIENTRY_DELAYED);

  altsetup->callbacks [ALT_CB_TARGET_DIR] = callbackInit (
      altsetupTargetDirDialog, altsetup, NULL);
  uibutton = uiCreateButton (
      altsetup->callbacks [ALT_CB_TARGET_DIR],
      "", NULL);
  altsetup->buttons [ALT_BUTTON_TARGET_DIR] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiCreateColonLabelOld (&uiwidget,
      /* CONTEXT: set up alternate: name (for shortcut) */
      _("Name"));
  uiBoxPackStart (hbox, &uiwidget);

  uiEntryCreate (altsetup->nameEntry);
  uiEntrySetValue (altsetup->nameEntry, "BDJ4 B");
  uiwidgetp = uiEntryGetWidgetContainer (altsetup->nameEntry);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: set up alternate: checkbox: re-install alternate */
  altsetup->reinstWidget = uiCreateCheckButton (_("Re-Install"),
      altsetup->reinstall);
  uiBoxPackStart (hbox, altsetup->reinstWidget);
  altsetup->callbacks [ALT_CB_REINST] = callbackInit (
      altsetupCheckDirTarget, altsetup, NULL);
  uiToggleButtonSetCallback (altsetup->reinstWidget, altsetup->callbacks [ALT_CB_REINST]);

  uiCreateLabelOld (&altsetup->feedbackMsg, "");
  uiWidgetSetClass (&altsetup->feedbackMsg, INST_HL_CLASS);
  uiBoxPackStart (hbox, &altsetup->feedbackMsg);

  uiwidgetp = uiCreateHorizSeparator ();
  uiWidgetSetClass (uiwidgetp, INST_SEP_CLASS);
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* button box */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uibutton = uiCreateButton (
      altsetup->callbacks [ALT_CB_EXIT],
      /* CONTEXT: set up alternate: exits the altsetup */
      _("Exit"), NULL);
  altsetup->buttons [ALT_BUTTON_EXIT] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackEnd (hbox, uiwidgetp);

  altsetup->callbacks [ALT_CB_START] = callbackInit (
      altsetupSetupCallback, altsetup, NULL);
  uibutton = uiCreateButton (
      altsetup->callbacks [ALT_CB_START],
      /* CONTEXT: set up alternate: start the set-up process */
      _("Start"), NULL);
  altsetup->buttons [ALT_BUTTON_START] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackEnd (hbox, uiwidgetp);

  altsetup->disptb = uiTextBoxCreate (200, NULL);
  uiTextBoxSetReadonly (altsetup->disptb);
  uiTextBoxHorizExpand (altsetup->disptb);
  uiTextBoxVertExpand (altsetup->disptb);
  uiBoxPackStartExpand (vbox, uiTextBoxGetScrolledWindow (altsetup->disptb));

  uiWidgetShowAll (altsetup->window);
  altsetup->uiBuilt = true;

  uiwcontFree (vbox);
  uiwcontFree (hbox);
}

static int
altsetupMainLoop (void *udata)
{
  altsetup_t *altsetup = udata;

  if (altsetup->guienabled) {
    uiUIProcessEvents ();

    uiEntryValidate (altsetup->targetEntry, false);

    if (altsetup->scrolltoend) {
      uiTextBoxScrollToEnd (altsetup->disptb);
      altsetup->scrolltoend = false;
      uiUIProcessWaitEvents ();
      /* go through the main loop once more */
      return TRUE;
    }
  }

  switch (altsetup->instState) {
    case ALT_PRE_INIT: {
      altsetup->instState = ALT_PREPARE;
      break;
    }
    case ALT_PREPARE: {
      if (altsetup->guienabled) {
        uiEntryValidate (altsetup->targetEntry, true);
      }
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

      logStart ("bdj4altsetup", "alt",
          LOG_IMPORTANT | LOG_BASIC | LOG_MAIN | LOG_REDIR_INST);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "=== alternate setup started");
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

  if (! altsetup->guienabled) {
    return UIENTRY_ERROR;
  }

  if (! altsetup->uiBuilt) {
    return UIENTRY_RESET;
  }

  dir = uiEntryGetValue (altsetup->targetEntry);
  tbool = uiToggleButtonIsActive (altsetup->reinstWidget);
  altsetup->newinstall = false;
  altsetup->reinstall = tbool;

  exists = fileopIsDirectory (dir);

  strlcpy (tbuff, dir, sizeof (tbuff));
  if (fileopIsDirectory (tbuff)) {
    strlcat (tbuff, "/data", sizeof (tbuff));
    if (! fileopIsDirectory (tbuff)) {
      exists = false;
      /* if there is an existing directory, the data/ sub-dir must exist */
      rc = UIENTRY_ERROR;
    }
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

  if (! *dir) {
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

  selectdata = uiDialogCreateSelect (altsetup->window,
      /* CONTEXT: set up alternate: dialog title for selecting location */
      _("Alternate Location"),
      uiEntryGetValue (altsetup->targetEntry), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    char        tbuff [MAXPATHLEN];

    strlcpy (tbuff, fn, sizeof (tbuff));
    /* validation gets called again upon set */
    uiEntrySetValue (altsetup->targetEntry, tbuff);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected target loc: %s", altsetup->target);
    mdfree (fn);
  }
  mdfree (selectdata);
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
  altsetup->reinstall = uiToggleButtonIsActive (altsetup->reinstWidget);
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
  /* CONTEXT: set up alternate: status message */
  altsetupDisplayText (altsetup, "-- ", _("Copying template files."), false);

  if (chdir (altsetup->target)) {
    altsetupFailWorkingDir (altsetup, altsetup->target);
    return;
  }

  instutilCopyTemplates ();
  instutilCopyHttpFiles ();
  templateImageCopy (NULL);

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

// ### FIX
// ### if the source dir is read-only, the home datatopdir should be
// ### used.
  pathbldMakePath (buff, sizeof (buff),
      "data/altcount", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_MAIN);

  /* read the current altcount */
  altcount = 0;
  fh = fileopOpen (buff, "r");
  if (fh != NULL) {
    (void) ! fgets (str, sizeof (str), fh);
    stringTrim (str);
    fclose (fh);
    altcount = atoi (str);
    /* update alternate count */
    ++altcount;
  }

  /* write the new altcount */
  fh = fileopOpen (buff, "w");
  if (fh != NULL) {
    snprintf (str, sizeof (str), "%d\n", altcount);
    fputs (str, fh);
    fclose (fh);
  }

  /* calculate the new base port */
  baseport = sysvarsGetNum (SVL_BASEPORT);
  baseport += altcount * BDJOPT_MAX_PROFILES * (int) bdjvarsGetNum (BDJVL_NUM_PORTS);
  snprintf (str, sizeof (str), "%d\n", baseport);

  /* write the new base port out */
  pathbldMakePath (buff, sizeof (buff),
      BASE_PORT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  fh = fileopOpen (buff, "w");
  if (fh != NULL) {
    fputs (str, fh);
    fclose (fh);
  }

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
// ### FIX these need to be in a known location if the main installation
// ### is read-only.
  pathbldMakePath (buff, sizeof (buff),
      VOLREG_FN, BDJ4_LINK_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "data/volreg", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_MAIN);
  fh = fileopOpen (buff, "w");
  if (fh != NULL) {
    fputs (tbuff, fh);
    fputs ("\n", fh);
    fclose (fh);
  }

  pathbldMakePath (buff, sizeof (buff),
      "volreglock", BDJ4_LINK_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "tmp/volreg", BDJ4_LOCK_EXT, PATHBLD_MP_DIR_MAIN);
  fh = fileopOpen (buff, "w");
  if (fh != NULL) {
    fputs (tbuff, fh);
    fputs ("\n", fh);
    fclose (fh);
  }

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
    uiwcontFree (altsetup->window);
    uiwcontFree (altsetup->reinstWidget);
    for (int i = 0; i < ALT_CB_MAX; ++i) {
      callbackFree (altsetup->callbacks [i]);
    }
    for (int i = 0; i < ALT_BUTTON_MAX; ++i) {
      uiButtonFree (altsetup->buttons [i]);
    }
    uiEntryFree (altsetup->nameEntry);
    uiEntryFree (altsetup->targetEntry);
    mdfree (altsetup->target);
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
  altsetup->target = mdstrdup (fn);
  pathNormPath (altsetup->target, strlen (altsetup->target));
}

