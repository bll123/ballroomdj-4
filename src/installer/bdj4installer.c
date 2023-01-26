/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#if _sys_xattr
# include <sys/xattr.h>
#endif
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
#include "datafile.h"
#include "dirlist.h"
#include "dirop.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "instutil.h"
#include "localeutil.h"
#include "locatebdj3.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "ossignal.h"
#include "osutils.h"
#include "osuiutils.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "callback.h"
#include "webclient.h"

/* installation states */
typedef enum {
  INST_INITIALIZE,
  INST_VERIFY_INST_INIT,
  INST_VERIFY_INSTALL,
  INST_PREPARE,
  INST_WAIT_USER,
  INST_INIT,
  INST_SAVE_TARGET,
  INST_MAKE_TARGET,
  INST_COPY_START,
  INST_COPY_FILES,
  INST_MAKE_DATA_TOP,
  INST_CREATE_DIRS,
  INST_COPY_TEMPLATES_INIT,
  INST_COPY_TEMPLATES,
  INST_CONVERT_START,
  INST_CONVERT,
  INST_CONVERT_FINISH,
  INST_CREATE_SHORTCUT,
  INST_VLC_CHECK,
  INST_VLC_DOWNLOAD,
  INST_VLC_INSTALL,
  INST_PYTHON_CHECK,
  INST_PYTHON_DOWNLOAD,
  INST_PYTHON_INSTALL,
  INST_MUTAGEN_CHECK,
  INST_MUTAGEN_INSTALL,
  INST_UPDATE_PROCESS_INIT,
  INST_UPDATE_PROCESS,
  INST_FINALIZE,
  INST_REGISTER_INIT,
  INST_REGISTER,
  INST_FINISH,
  INST_EXIT,
} installstate_t;

enum {
  INST_CB_TARGET_DIR,
  INST_CB_BDJ3LOC_DIR,
  INST_CB_EXIT,
  INST_CB_INSTALL,
  INST_CB_REINST,
  INST_CB_CONV,
  INST_CB_MAX,
};

enum {
  INST_BUTTON_TARGET_DIR,
  INST_BUTTON_BDJ3LOC_DIR,
  INST_BUTTON_EXIT,
  INST_BUTTON_INSTALL,
  INST_BUTTON_MAX,
};

enum {
  INST_TARGET,
  INST_BDJ3LOC,
};

#define INST_DISP_ACTION "-- "
#define INST_DISP_STATUS "   "
#define INST_DISP_ERROR "** "

#define BDJ4_MACOS_DIR    "BDJ4.app"
#define MACOS_PREFIX      "/Contents/MacOS"

typedef struct {
  installstate_t  instState;
  installstate_t  lastInstState;            // debugging
  callback_t      *callbacks [INST_CB_MAX];
  char            *home;
  char            *target;
  char            *hostname;
  char            rundir [MAXPATHLEN];
  char            datatopdir [MAXPATHLEN];
  char            currdir [MAXPATHLEN];
  char            unpackdir [MAXPATHLEN];
  char            vlcversion [40];
  char            pyversion [40];
  char            dlfname [MAXPATHLEN];
  char            oldversion [MAXPATHLEN];
  char            bdj3version [MAXPATHLEN];
  loglevel_t      loglevel;
  webclient_t     *webclient;
  char            *webresponse;
  size_t          webresplen;
  const char      *pleasewaitmsg;
  /* conversion */
  char            *bdj3loc;
  char            *tclshloc;
  slist_t         *convlist;
  slistidx_t      convidx;
  UIWidget        window;
  UIWidget        statusMsg;
  uientry_t       *targetEntry;
  UIWidget        reinstWidget;
  UIWidget        feedbackMsg;
  uientry_t       *bdj3locEntry;
  UIWidget        convWidget;
  UIWidget        convFeedbackMsg;
  UIWidget        vlcMsg;
  UIWidget        pythonMsg;
  UIWidget        mutagenMsg;
  uitextbox_t     *disptb;
  uibutton_t      *buttons [INST_BUTTON_MAX];
  /* flags */
  bool            convprocess : 1;
  bool            guienabled : 1;
  bool            inSetConvert : 1;
  bool            newinstall : 1;
  bool            nodatafiles : 1;
  bool            pythoninstalled : 1;
  bool            quiet : 1;
  bool            reinstall : 1;
  bool            scrolltoend : 1;
  bool            testregistration : 1;
  bool            uiBuilt : 1;
  bool            unattended : 1;
  bool            updatepython : 1;
  bool            verbose : 1;
  bool            vlcinstalled : 1;
} installer_t;

#define INST_HL_COLOR "#b16400"
#define INST_HL_CLASS "insthl"
#define INST_SEP_COLOR "#733000"
#define INST_SEP_CLASS "instsep"

#define INST_NEW_FILE "data/newinstall.txt"
#define INST_TEMP_FILE  "tmp/bdj4instout.txt"
#define INST_SAVE_FNAME "installdir.txt"
#define CONV_TEMP_FILE "tmp/bdj4convout.txt"
#define BDJ3_LOC_FILE "install/bdj3loc.txt"

static void installerBuildUI (installer_t *installer);
static int  installerMainLoop (void *udata);
static bool installerExitCallback (void *udata);
static bool installerCheckDirTarget (void *udata);
static bool installerCheckDirConv (void *udata);
static bool installerTargetDirDialog (void *udata);
static void installerSetBDJ3LocEntry (installer_t *installer, const char *bdj3loc);
static bool installerBDJ3LocDirDialog (void *udata);
static int  installerValidateTarget (uientry_t *entry, void *udata);
static int  installerValidateProcessTarget (installer_t *installer, const char *dir);
static int  installerValidateBDJ3Loc (uientry_t *entry, void *udata);
static void installerSetConvert (installer_t *installer, int val);
static void installerDisplayConvert (installer_t *installer);
static bool installerInstallCallback (void *udata);
static bool installerCheckTarget (installer_t *installer, const char *dir);
static void installerSetPaths (installer_t *installer);

static void installerVerifyInstInit (installer_t *installer);
static void installerVerifyInstall (installer_t *installer);
static void installerInstInit (installer_t *installer);
static void installerSaveTargetDir (installer_t *installer);
static void installerMakeTarget (installer_t *installer);
static void installerCopyStart (installer_t *installer);
static void installerCopyFiles (installer_t *installer);
static void installerMakeDataTop (installer_t *installer);
static void installerCreateDirs (installer_t *installer);
static void installerCopyTemplatesInit (installer_t *installer);
static void installerCopyTemplates (installer_t *installer);
static void installerConvertStart (installer_t *installer);
static void installerConvert (installer_t *installer);
static void installerConvertFinish (installer_t *installer);
static void installerCreateShortcut (installer_t *installer);
static void installerUpdateProcessInit (installer_t *installer);
static void installerUpdateProcess (installer_t *installer);
static void installerVLCCheck (installer_t *installer);
static void installerVLCDownload (installer_t *installer);
static void installerVLCInstall (installer_t *installer);
static void installerPythonCheck (installer_t *installer);
static void installerPythonDownload (installer_t *installer);
static void installerPythonInstall (installer_t *installer);
static void installerMutagenCheck (installer_t *installer);
static void installerMutagenInstall (installer_t *installer);
static void installerFinalize (installer_t *installer);
static void installerRegisterInit (installer_t *installer);
static void installerRegister (installer_t *installer);

static void installerCleanup (installer_t *installer);
static void installerDisplayText (installer_t *installer, const char *pfx, const char *txt, bool bold);
static void installerGetTargetSaveFname (installer_t *installer, char *buff, size_t len);
static void installerGetBDJ3Fname (installer_t *installer, char *buff, size_t len);
static void installerTemplateCopy (const char *dir, const char *from, const char *to);
static void installerSetrundir (installer_t *installer, const char *dir);
static void installerVLCGetVersion (installer_t *installer);
static void installerPythonGetVersion (installer_t *installer);
static void installerCheckPackages (installer_t *installer);
static void installerWebResponseCallback (void *userdata, char *resp, size_t len);
static void installerFailWorkingDir (installer_t *installer, const char *dir, const char *msg);
static void installerSetTargetDir (installer_t *installer, const char *fn);
static void installerSetBDJ3LocDir (installer_t *installer, const char *fn);
static void installerCheckAndFixTarget (char *buff, size_t sz);
static bool installerWinVerifyProcess (installer_t *installer);

int
main (int argc, char *argv[])
{
  installer_t   installer;
  char          tbuff [512];
  char          buff [MAXPATHLEN];
  FILE          *fh;
  int           c = 0;
  int           option_index = 0;

  static struct option bdj_options [] = {
    { "bdj3dir",    required_argument,  NULL,   '3' },
    { "bdj4installer",no_argument,      NULL,   0 },
    { "nodatafiles", no_argument,       NULL,   'N' },
    { "reinstall",  no_argument,        NULL,   'r' },
    { "targetdir",  required_argument,  NULL,   't' },
    { "testregistration", no_argument,  NULL,   'T' },
    { "unattended", no_argument,        NULL,   'U' },
    { "unpackdir",  required_argument,  NULL,   'u' },
    { "locale",     required_argument,  NULL,   'L' },
    /* generic args */
    { "cli",        no_argument,        NULL,   'C' },
    { "quiet"  ,    no_argument,        NULL,   'Q' },
    { "verbose",    no_argument,        NULL,   'V' },
    /* bdj4 launcher args */
    { "bdj4",       no_argument,        NULL,   0 },
    { "debug",      required_argument,  NULL,   'd' },
    { "debugself",  no_argument,        NULL,   0 },
    { "msys",       no_argument,        NULL,   0 },
    { "nodetach",   no_argument,        NULL,   0 },
    { "theme",      required_argument,  NULL,   0 },
    { "wait",       no_argument,        NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };


#if BDJ4_MEM_DEBUG
  mdebugInit ("inst");
#endif

  buff [0] = '\0';

  installer.unpackdir [0] = '\0';
  installer.home = NULL;
  uiutilsUIWidgetInit (&installer.window);
  installer.instState = INST_INITIALIZE;
  installer.lastInstState = INST_INITIALIZE;
  installer.target = mdstrdup ("");
  installer.rundir [0] = '\0';
  installer.bdj3loc = mdstrdup ("");
  installer.convidx = 0;
  installer.convlist = NULL;
  installer.tclshloc = NULL;
  installer.currdir [0] = '\0';
  installer.disptb = NULL;
  /* CONTEXT: installer: status message */
  installer.pleasewaitmsg = _("Please wait\xe2\x80\xa6");

  installer.convprocess = false;
  installer.guienabled = true;
  installer.inSetConvert = false;
  installer.newinstall = true;
  installer.nodatafiles = false;
  installer.pythoninstalled = false;
  installer.quiet = false;
  installer.reinstall = false;
  installer.scrolltoend = false;
  installer.testregistration = false;
  installer.uiBuilt = false;
  installer.unattended = false;
  installer.updatepython = false;
  installer.verbose = false;
  installer.vlcinstalled = false;
  for (int i = 0; i < INST_CB_MAX; ++i) {
    installer.callbacks [i] = NULL;
  }

  installer.loglevel = LOG_IMPORTANT | LOG_BASIC | LOG_MAIN | LOG_REDIR_INST;
  uiutilsUIWidgetInit (&installer.statusMsg);
  uiutilsUIWidgetInit (&installer.reinstWidget);
  uiutilsUIWidgetInit (&installer.feedbackMsg);
  uiutilsUIWidgetInit (&installer.convFeedbackMsg);
  uiutilsUIWidgetInit (&installer.vlcMsg);
  uiutilsUIWidgetInit (&installer.pythonMsg);
  uiutilsUIWidgetInit (&installer.mutagenMsg);
  uiutilsUIWidgetInit (&installer.convWidget);
  (void) ! getcwd (installer.currdir, sizeof (installer.currdir));
  installer.webclient = NULL;
  strcpy (installer.vlcversion, "");
  strcpy (installer.pyversion, "");
  strcpy (installer.oldversion, "");
  strcpy (installer.bdj3version, "");
  for (int i = 0; i < INST_BUTTON_MAX; ++i) {
    installer.buttons [i] = NULL;
  }
  installer.targetEntry = NULL;
  installer.bdj3locEntry = NULL;

  /* the data in sysvars will not be correct.  don't use it.  */
  /* the installer only needs the home, hostname, os info and locale */
  sysvarsInit (argv[0]);
  localeInit ();

  installer.hostname = sysvarsGetStr (SV_HOSTNAME);
  installer.home = sysvarsGetStr (SV_HOME);

  strlcpy (buff, installer.home, sizeof (buff));
  if (isMacOS ()) {
    snprintf (buff, sizeof (buff), "%s/Applications", installer.home);
  }
  installerCheckAndFixTarget (buff, sizeof (buff));

  installerGetTargetSaveFname (&installer, tbuff, sizeof (tbuff));
  fh = fileopOpen (tbuff, "r");
  if (fh != NULL) {
    (void) ! fgets (buff, sizeof (buff), fh);
    stringTrim (buff);
    fclose (fh);
  }

  /* at this point the target dir will have either a good default */
  /* or the saved target name */
  installerSetTargetDir (&installer, buff);

  while ((c = getopt_long_only (argc, argv, "Cru:l:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg) {
          installer.loglevel = (loglevel_t) atol (optarg);
          installer.loglevel |= LOG_REDIR_INST;
        }
        break;
      }
      case 'C': {
        installer.guienabled = false;
#if _define_SIGCHLD
        osDefaultSignal (SIGCHLD);
#endif
        break;
      }
      case 'T': {
        installer.testregistration = true;
        break;
      }
      case 'U': {
        installer.unattended = true;
        break;
      }
      case 'r': {
        installer.reinstall = true;
        break;
      }
      case 'u': {
        strlcpy (installer.unpackdir, optarg, sizeof (installer.unpackdir));
        break;
      }
      case 'V': {
        installer.verbose = true;
        break;
      }
      case 'Q': {
        installer.quiet = true;
        break;
      }
      case 't': {
        installerSetTargetDir (&installer, optarg);
        break;
      }
      case '3': {
        installerSetBDJ3LocDir (&installer, optarg);
        break;
      }
      case 'N': {
        installer.nodatafiles = true;
        break;
      }
      case 'L': {
        sysvarsSetStr (SV_LOCALE, optarg);
        snprintf (tbuff, sizeof (tbuff), "%.2s", optarg);
        sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
        sysvarsSetNum (SVL_LOCALE_SET, 1);
        break;
      }
      default: {
        break;
      }
    }
  }

  if (installer.guienabled) {
    installer.targetEntry = uiEntryInit (80, MAXPATHLEN);
    installer.bdj3locEntry = uiEntryInit (80, MAXPATHLEN);
  }

  if (installer.unattended) {
    bool    ok = true;
    bool    exists;

    exists = installerCheckTarget (&installer, installer.target);
    if (fileopIsDirectory (installer.target)) {
      if (! exists) {
        fprintf (stderr, "target directory is invalid: exists but not bdj4\n");
        ok = false;
      }
      if (exists && installer.newinstall && installer.reinstall) {
        fprintf (stderr, "target directory is invalid: new & reinstall\n");
        ok = false;
      }
    }
    if (*installer.bdj3loc) {
      if (! fileopIsDirectory (installer.bdj3loc)) {
        fprintf (stderr, "invalid bdj3 loc\n");
        ok = false;
      }
    }

    if (! ok) {
      fprintf (stdout, "finish NG\n");
      dataFree (installer.target);
      dataFree (installer.bdj3loc);
#if BDJ4_MEM_DEBUG
      mdebugReport ();
      mdebugCleanup ();
#endif
      exit (1);
    }
  }

  if (*installer.unpackdir == '\0') {
    fprintf (stderr, "Error: unpackdir argument is required\n");
    fprintf (stdout, "finish NG\n");
    dataFree (installer.target);
    dataFree (installer.bdj3loc);
#if BDJ4_MEM_DEBUG
    mdebugReport ();
    mdebugCleanup ();
#endif
    exit (1);
  }

  /* this only works if the installer.target is pointing at an existing */
  /* install of BDJ4 */
  installerGetBDJ3Fname (&installer, tbuff, sizeof (tbuff));
  if (*tbuff) {
    fh = fileopOpen (tbuff, "r");
    if (fh != NULL) {
      (void) ! fgets (buff, sizeof (buff), fh);
      stringTrim (buff);
      installerSetBDJ3LocDir (&installer, buff);
      fclose (fh);
    } else {
      char *fn;

      fn = locatebdj3 ();
      installerSetBDJ3LocDir (&installer, fn);
      mdfree (fn);
    }
  }

  if (isWindows ()) {
    pathWinPath (installer.target, strlen (installer.target));
  }

  if (installer.guienabled) {
    char *uifont;

    uiUIInitialize ();

    uifont = sysvarsGetStr (SV_FONT_DEFAULT);
    if (uifont == NULL || ! *uifont) {
      uifont = "Arial Regular 14";
      if (isMacOS ()) {
        uifont = "Arial Regular 17";
      }
    }
    uiSetUIFont (uifont, INST_HL_COLOR, NULL);
  }

  installerCheckPackages (&installer);

  if (installer.guienabled) {
    installerBuildUI (&installer);
    osuiFinalize ();
    /* to get initial feedback messages displayed */
    installerValidateProcessTarget (&installer, installer.target);
  } else {
    installer.instState = INST_INIT;
  }

  while (installer.instState != INST_EXIT) {
    if (installer.instState != installer.lastInstState) {
      if (installer.nodatafiles) {
        if (installer.verbose && ! installer.quiet) {
          fprintf (stderr, "state: %d\n", installer.instState);
        }
      } else {
        logMsg (LOG_INSTALL, LOG_IMPORTANT, "state: %d", installer.instState);
      }
      installer.lastInstState = installer.instState;
    }
    installerMainLoop (&installer);
    mssleep (10);
  }

  /* process any final events */
  if (installer.guienabled) {
    uiUIProcessEvents ();
    uiCleanup ();
  }

  installerCleanup (&installer);
  localeCleanup ();
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif

  return 0;
}

static void
installerBuildUI (installer_t *installer)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  uibutton_t    *uibutton;
  UIWidget      *uiwidgetp;
  UIWidget      sg;
  char          tbuff [100];
  char          imgbuff [MAXPATHLEN];

  uiLabelAddClass (INST_HL_CLASS, INST_HL_COLOR);
  uiSeparatorAddClass (INST_SEP_CLASS, INST_SEP_COLOR);

  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&uiwidget);

  strlcpy (imgbuff, "img/bdj4_icon_inst.png", sizeof (imgbuff));
  osuiSetIcon (imgbuff);

  strlcpy (imgbuff, "img/bdj4_icon_inst.svg", sizeof (imgbuff));
  /* CONTEXT: installer: window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Installer"), BDJ4_NAME);
  installer->callbacks [INST_CB_EXIT] = callbackInit (
      installerExitCallback, installer, NULL);
  uiCreateMainWindow (&installer->window,
      installer->callbacks [INST_CB_EXIT],
      tbuff, imgbuff);
  uiWindowSetDefaultSize (&installer->window, 1000, 600);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 10);
  uiWidgetSetMarginTop (&vbox, 20);
  uiWidgetExpandHoriz (&vbox);
  uiWidgetExpandVert (&vbox);
  uiBoxPackInWindow (&installer->window, &vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&installer->statusMsg, "");
  uiWidgetAlignHorizEnd (&installer->statusMsg);
  uiWidgetSetClass (&installer->statusMsg, INST_HL_CLASS);
  uiBoxPackEndExpand (&hbox, &installer->statusMsg);

  uiCreateLabel (&uiwidget,
      /* CONTEXT: installer: where BDJ4 gets installed */
      _("Enter the destination folder where BDJ4 will be installed."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiEntryCreate (installer->targetEntry);
  uiEntrySetValue (installer->targetEntry, installer->target);
  uiwidgetp = uiEntryGetUIWidget (installer->targetEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (&hbox, uiwidgetp);
  uiEntrySetValidate (installer->targetEntry,
      installerValidateTarget, installer, UIENTRY_DELAYED);

  installer->callbacks [INST_CB_TARGET_DIR] = callbackInit (
      installerTargetDirDialog, installer, NULL);
  uibutton = uiCreateButton (
      installer->callbacks [INST_CB_TARGET_DIR],
      "", NULL);
  installer->buttons [INST_BUTTON_TARGET_DIR] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: installer: checkbox: re-install BDJ4 */
  uiCreateCheckButton (&installer->reinstWidget, _("Re-Install"),
      installer->reinstall);
  uiBoxPackStart (&hbox, &installer->reinstWidget);
  installer->callbacks [INST_CB_REINST] = callbackInit (
      installerCheckDirTarget, installer, NULL);
  uiToggleButtonSetCallback (&installer->reinstWidget,
      installer->callbacks [INST_CB_REINST]);

  uiCreateLabel (&installer->feedbackMsg, "");
  uiWidgetSetClass (&installer->feedbackMsg, INST_HL_CLASS);
  uiBoxPackStart (&hbox, &installer->feedbackMsg);

  uiCreateHorizSeparator (&uiwidget);
  uiWidgetSetClass (&uiwidget, INST_SEP_CLASS);
  uiBoxPackStart (&vbox, &uiwidget);

  /* conversion process */
  snprintf (tbuff, sizeof (tbuff),
      /* CONTEXT: installer: asking where BallroomDJ 3 is installed */
      _("Enter the folder where %s is installed."), BDJ3_NAME);
  uiCreateLabel (&uiwidget, tbuff);
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateLabel (&uiwidget,
      /* CONTEXT: installer: instructions */
      _("If there is no BallroomDJ 3 installation, leave the entry blank."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateLabel (&uiwidget,
      /* CONTEXT: installer: instructions */
      _("The conversion process will only run for new installations and for re-installations."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: installer: label for entry field asking for BDJ3 location */
  snprintf (tbuff, sizeof (tbuff), _("%s Location"), BDJ3_NAME);
  uiCreateColonLabel (&uiwidget, tbuff);
  uiBoxPackStart (&hbox, &uiwidget);

  uiEntryCreate (installer->bdj3locEntry);
  installerSetBDJ3LocEntry (installer, installer->bdj3loc);
  uiwidgetp = uiEntryGetUIWidget (installer->bdj3locEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (&hbox, uiwidgetp);
  uiEntrySetValidate (installer->bdj3locEntry,
      installerValidateBDJ3Loc, installer, UIENTRY_DELAYED);

  installer->callbacks [INST_CB_BDJ3LOC_DIR] = callbackInit (
      installerBDJ3LocDirDialog, installer, NULL);
  uibutton = uiCreateButton (
      installer->callbacks [INST_CB_BDJ3LOC_DIR],
      "", NULL);
  installer->buttons [INST_BUTTON_BDJ3LOC_DIR] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: installer: checkbox: convert the BallroomDJ 3 installation */
  snprintf (tbuff, sizeof (tbuff), _("Convert %s"), BDJ3_NAME);
  uiCreateCheckButton (&installer->convWidget, tbuff, 0);
  uiBoxPackStart (&hbox, &installer->convWidget);
  installer->callbacks [INST_CB_CONV] = callbackInit (
      installerCheckDirConv, installer, NULL);
  uiToggleButtonSetCallback (&installer->convWidget,
      installer->callbacks [INST_CB_CONV]);

  uiCreateLabel (&installer->convFeedbackMsg, "");
  uiWidgetSetClass (&installer->convFeedbackMsg, INST_HL_CLASS);
  uiBoxPackStart (&hbox, &installer->convFeedbackMsg);

  /* VLC status */

  uiCreateHorizSeparator (&uiwidget);
  uiWidgetSetClass (&uiwidget, INST_SEP_CLASS);
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateSizeGroupHoriz (&sg);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget, "VLC");
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiCreateLabel (&installer->vlcMsg, "");
  uiWidgetSetClass (&installer->vlcMsg, INST_HL_CLASS);
  uiBoxPackStart (&hbox, &installer->vlcMsg);

  /* python status */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget, "Python");
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiCreateLabel (&installer->pythonMsg, "");
  uiWidgetSetClass (&installer->pythonMsg, INST_HL_CLASS);
  uiBoxPackStart (&hbox, &installer->pythonMsg);

  /* mutagen status */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget, "Mutagen");
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiCreateLabel (&installer->mutagenMsg, "");
  uiWidgetSetClass (&installer->mutagenMsg, INST_HL_CLASS);
  uiBoxPackStart (&hbox, &installer->mutagenMsg);

  /* button box */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uibutton = uiCreateButton (
      installer->callbacks [INST_CB_EXIT],
      /* CONTEXT: installer: exits the installer */
      _("Exit"), NULL);
  installer->buttons [INST_BUTTON_EXIT] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackEnd (&hbox, uiwidgetp);

  installer->callbacks [INST_CB_INSTALL] = callbackInit (
      installerInstallCallback, installer, NULL);
  uibutton = uiCreateButton (
      installer->callbacks [INST_CB_INSTALL],
      /* CONTEXT: installer: start the installation process */
      _("Install"), NULL);
  installer->buttons [INST_BUTTON_INSTALL] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackEnd (&hbox, uiwidgetp);

  installer->disptb = uiTextBoxCreate (300, INST_HL_COLOR);
  uiTextBoxSetReadonly (installer->disptb);
  uiTextBoxHorizExpand (installer->disptb);
  uiTextBoxVertExpand (installer->disptb);
  uiBoxPackStartExpand (&vbox, uiTextBoxGetScrolledWindow (installer->disptb));

  uiWidgetShowAll (&installer->window);
  installer->uiBuilt = true;
}

static int
installerMainLoop (void *udata)
{
  installer_t *installer = udata;

  if (installer->guienabled) {
    uiUIProcessEvents ();
  }

  uiEntryValidate (installer->targetEntry, false);
  uiEntryValidate (installer->bdj3locEntry, false);

  if (installer->guienabled && installer->scrolltoend) {
    uiTextBoxScrollToEnd (installer->disptb);
    installer->scrolltoend = false;
    uiUIProcessWaitEvents ();
    /* go through the main loop once more */
    return TRUE;
  }

  switch (installer->instState) {
    case INST_INITIALIZE: {
      if (isWindows ()) {
        /* windows checksum verification is too slow, don't use it */
        installer->instState = INST_PREPARE;
      } else {
        installer->instState = INST_VERIFY_INST_INIT;
      }
      break;
    }
    case INST_VERIFY_INST_INIT: {
      installerVerifyInstInit (installer);
      break;
    }
    case INST_VERIFY_INSTALL: {
      installerVerifyInstall (installer);
      break;
    }
    case INST_PREPARE: {
      installerDisplayConvert (installer);
      installerCheckPackages (installer);

      uiEntryValidate (installer->targetEntry, true);
      uiEntryValidate (installer->bdj3locEntry, true);
      installer->instState = INST_WAIT_USER;
      break;
    }
    case INST_WAIT_USER: {
      /* do nothing */
      break;
    }
    case INST_INIT: {
      installerInstInit (installer);
      break;
    }
    case INST_SAVE_TARGET: {
      installerSaveTargetDir (installer);
      break;
    }
    case INST_MAKE_TARGET: {
      installerMakeTarget (installer);
      break;
    }
    case INST_COPY_START: {
      installerCopyStart (installer);
      break;
    }
    case INST_COPY_FILES: {
      installerCopyFiles (installer);
      break;
    }
    case INST_MAKE_DATA_TOP: {
      installerMakeDataTop (installer);
      break;
    }
    case INST_CREATE_DIRS: {
      installerCreateDirs (installer);

      logStart ("bdj4installer", "inst", installer->loglevel);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "target: %s", installer->target);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "initial bdj3loc: %s", installer->bdj3loc);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "new-install: %d", installer->newinstall);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "re-install: %d", installer->reinstall);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "convert: %d", installer->convprocess);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "vlc-inst: %d", installer->vlcinstalled);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "python-inst: %d", installer->pythoninstalled);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "python-upd: %d", installer->updatepython);

      break;
    }
    case INST_COPY_TEMPLATES_INIT: {
      installerCopyTemplatesInit (installer);
      break;
    }
    case INST_COPY_TEMPLATES: {
      installerCopyTemplates (installer);
      break;
    }
    case INST_CONVERT_START: {
      installerConvertStart (installer);
      break;
    }
    case INST_CONVERT: {
      installerConvert (installer);
      break;
    }
    case INST_CONVERT_FINISH: {
      installerConvertFinish (installer);
      break;
    }
    case INST_CREATE_SHORTCUT: {
      installerCreateShortcut (installer);
      break;
    }
    case INST_VLC_CHECK: {
      installerVLCCheck (installer);
      break;
    }
    case INST_VLC_DOWNLOAD: {
      installerVLCDownload (installer);
      break;
    }
    case INST_VLC_INSTALL: {
      installerVLCInstall (installer);
      break;
    }
    case INST_PYTHON_CHECK: {
      installerPythonCheck (installer);
      break;
    }
    case INST_PYTHON_DOWNLOAD: {
      installerPythonDownload (installer);
      break;
    }
    case INST_PYTHON_INSTALL: {
      installerPythonInstall (installer);
      break;
    }
    case INST_MUTAGEN_CHECK: {
      installerMutagenCheck (installer);
      break;
    }
    case INST_MUTAGEN_INSTALL: {
      installerMutagenInstall (installer);
      break;
    }
    case INST_UPDATE_PROCESS_INIT: {
      installerUpdateProcessInit (installer);
      break;
    }
    case INST_UPDATE_PROCESS: {
      installerUpdateProcess (installer);
      break;
    }
    case INST_FINALIZE: {
      installerFinalize (installer);
      break;
    }
    case INST_REGISTER_INIT: {
      installerRegisterInit (installer);
      break;
    }
    case INST_REGISTER: {
      installerRegister (installer);
      break;
    }
    case INST_FINISH: {
      /* CONTEXT: installer: status message */
      installerDisplayText (installer, "## ",  _("Installation complete."), true);
      if (installer->guienabled) {
        installer->instState = INST_PREPARE;
      } else {
        installer->instState = INST_EXIT;
      }
      break;
    }
    case INST_EXIT: {
      break;
    }
  }

  return TRUE;
}

static bool
installerCheckDirTarget (void *udata)
{
  installer_t   *installer = udata;

  if (installer->inSetConvert) {
    return UICB_STOP;
  }

  uiEntryValidate (installer->targetEntry, true);
  /* validating the target may change the conversion toggle button */
  uiEntryValidate (installer->bdj3locEntry, true);
  return UICB_CONT;
}

static bool
installerCheckDirConv (void *udata)
{
  installer_t   *installer = udata;

  if (installer->inSetConvert) {
    return UICB_STOP;
  }

  uiEntryValidate (installer->bdj3locEntry, true);
  return UICB_CONT;
}

static int
installerValidateTarget (uientry_t *entry, void *udata)
{
  installer_t   *installer = udata;
  const char    *dir;
  bool          tbool;

  if (! installer->guienabled) {
    return UIENTRY_ERROR;
  }

  if (! installer->uiBuilt) {
    return UIENTRY_RESET;
  }

  dir = uiEntryGetValue (installer->targetEntry);
  tbool = uiToggleButtonIsActive (&installer->reinstWidget);
  installer->reinstall = tbool;
  if (installer->newinstall) {
    installer->reinstall = false;
  }

  if (strcmp (dir, installer->target) == 0) {
    /* no change */
    /* prevent the convprocess flag from bouncing between different states */
    return UIENTRY_OK;
  }

  return installerValidateProcessTarget (installer, dir);
}

static int
installerValidateProcessTarget (installer_t *installer, const char *dir)
{
  bool          exists = false;
  char          tbuff [MAXPATHLEN];
  int           rc = UIENTRY_OK;
  bool          found = false;

  /* base possibilities: */
  /*  a) exists, has a bdj4 installation (r/u) */
  /*  b) exists, no bdj4, append bdj4-name, has bdj4 installation (r/u) */
  /*  c) exists, no bdj4, append bdj4-name, no bdj4 (f) */
  /*  d) does not exist, append bdj4-name (n) */
  /* r/u = re-install/update */
  /* f = fail */
  /* n = new */
  /* in cases b, c, and d the bdj4-name should be appended */

  exists = fileopIsDirectory (dir);

  if (exists) {
    found = installerCheckTarget (installer, dir);
    if (! found) {
      strlcpy (tbuff, dir, sizeof (tbuff));
      installerCheckAndFixTarget (tbuff, sizeof (tbuff));
      exists = fileopIsDirectory (tbuff);
      if (exists) {
        /* cannot set the display, as the user may be typing */
        installerSetTargetDir (installer, tbuff);
        found = installerCheckTarget (installer, tbuff);
      }
    } else {
      installerSetTargetDir (installer, dir);
    }
  } else {
    strlcpy (tbuff, dir, sizeof (tbuff));
    installerCheckAndFixTarget (tbuff, sizeof (tbuff));
    found = installerCheckTarget (installer, tbuff);
    /* cannot set the display, as the user may be typing */
    installerSetTargetDir (installer, tbuff);
  }

  if (exists) {
    if (found) {
      if (installer->reinstall) {
        /* CONTEXT: installer: message indicating the action that will be taken */
        snprintf (tbuff, sizeof (tbuff), _("Re-install %s."), BDJ4_NAME);
        uiLabelSetText (&installer->feedbackMsg, tbuff);
        installerSetConvert (installer, TRUE);
      } else {
        /* CONTEXT: installer: message indicating the action that will be taken */
        snprintf (tbuff, sizeof (tbuff), _("Updating existing %s installation."), BDJ4_NAME);
        uiLabelSetText (&installer->feedbackMsg, tbuff);
        installerSetConvert (installer, FALSE);
      }
    } else {
      /* CONTEXT: installer: the selected folder exists and is not a BDJ4 installation */
      uiLabelSetText (&installer->feedbackMsg, _("Error: Folder already exists."));
      installerSetConvert (installer, FALSE);
      rc = UIENTRY_ERROR;
    }
  }
  if (! exists) {
    /* CONTEXT: installer: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("New %s installation."), BDJ4_NAME);
    uiLabelSetText (&installer->feedbackMsg, tbuff);
    installerSetConvert (installer, TRUE);
  }

  if (rc == UIENTRY_OK) {
    installerSetPaths (installer);
  }
  return rc;
}

static int
installerValidateBDJ3Loc (uientry_t *entry, void *udata)
{
  installer_t   *installer = udata;
  bool          locok = false;
  char          tbuff [200];
  int           rc = UIENTRY_OK;

  if (! installer->guienabled) {
    return UIENTRY_ERROR;
  }

  if (! installer->uiBuilt) {
    return UIENTRY_RESET;
  }

  /* bdj3 location validation */

  strlcpy (tbuff, uiEntryGetValue (installer->bdj3locEntry), sizeof (tbuff));
  pathNormPath (tbuff, strlen (tbuff));
  if (*tbuff == '\0' || strcmp (tbuff, "-") == 0) {
    locok = true;
  } else {
    if (! isMacOS ()) {
      if (locationcheck (tbuff)) {
        locok = true;
      }
    } else {
      char  *fn;

      fn = locatebdj3 ();
      if (fn != NULL) {
        locok = true;
        mdfree (fn);
      }
    }
  }

  if (! locok) {
    rc = UIENTRY_ERROR;
    /* CONTEXT: installer: the location entered is not a valid BDJ3 location. */
    snprintf (tbuff, sizeof (tbuff), _("Not a valid %s folder."), BDJ3_NAME);
    uiLabelSetText (&installer->convFeedbackMsg, tbuff);
    installerSetConvert (installer, FALSE);
  }

  /* will call display-convert */
  installerSetPaths (installer);
  return rc;
}

static bool
installerTargetDirDialog (void *udata)
{
  installer_t *installer = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;

  selectdata = uiDialogCreateSelect (&installer->window,
      /* CONTEXT: installer: dialog title for selecting install location */
      _("Install Location"),
      uiEntryGetValue (installer->targetEntry),
      NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    char        tbuff [MAXPATHLEN];

    strlcpy (tbuff, fn, sizeof (tbuff));
    uiEntrySetValue (installer->targetEntry, tbuff);
    mdfree (fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected target loc: %s", installer->target);
  }
  mdfree (selectdata);
  return UICB_CONT;
}

static void
installerSetBDJ3LocEntry (installer_t *installer, const char *bdj3loc)
{
  char    tbuff [MAXPATHLEN];

  strlcpy (tbuff, bdj3loc, sizeof (tbuff));
  if (isWindows ()) {
    pathWinPath (tbuff, sizeof (tbuff));
  }
  uiEntrySetValue (installer->bdj3locEntry, tbuff);
}

static bool
installerBDJ3LocDirDialog (void *udata)
{
  installer_t *installer = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;
  char        tbuff [100];

  /* CONTEXT: installer: dialog title for selecting BDJ3 location */
  snprintf (tbuff, sizeof (tbuff), _("Select %s Location"), BDJ3_NAME);
  selectdata = uiDialogCreateSelect (&installer->window,
      tbuff, uiEntryGetValue (installer->bdj3locEntry), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    installerSetBDJ3LocEntry (installer, fn);
    mdfree (fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected bdj3 loc: %s", installer->bdj3loc);
  }
  mdfree (selectdata);
  return UICB_CONT;
}

static void
installerSetConvert (installer_t *installer, int val)
{
  installer->inSetConvert = true;
  uiToggleButtonSetState (&installer->convWidget, val);
  installer->inSetConvert = false;
}

static void
installerDisplayConvert (installer_t *installer)
{
  int           nval;
  char          *tptr;
  char          tbuff [200];
  bool          nodir = false;

  nval = uiToggleButtonIsActive (&installer->convWidget);

  if (strcmp (installer->bdj3loc, "-") == 0 ||
      *installer->bdj3loc == '\0' ||
      installer->nodatafiles) {
    nval = 0;
    nodir = true;
    installerSetConvert (installer, nval);
  }

  installer->convprocess = nval;

  if (nval) {
    /* CONTEXT: installer: message indicating the conversion action that will be taken */
    tptr = _("%s data will be converted.");
    snprintf (tbuff, sizeof (tbuff), tptr, BDJ3_NAME);
    uiLabelSetText (&installer->convFeedbackMsg, tbuff);
  } else {
    if (nodir) {
      /* CONTEXT: installer: message indicating the conversion action that will be taken */
      tptr = _("No folder specified.");
      uiLabelSetText (&installer->convFeedbackMsg, tptr);
    } else {
      /* CONTEXT: installer: message indicating the conversion action that will be taken */
      tptr = _("The conversion process will not be run.");
      uiLabelSetText (&installer->convFeedbackMsg, tptr);
    }
  }
}

static bool
installerExitCallback (void *udata)
{
  installer_t   *installer = udata;

  installer->instState = INST_EXIT;
  return UICB_CONT;
}

static bool
installerInstallCallback (void *udata)
{
  installer_t *installer = udata;

  if (installer->instState == INST_WAIT_USER) {
    installer->instState = INST_INIT;
  }
  return UICB_CONT;
}

static bool
installerCheckTarget (installer_t *installer, const char *dir)
{
  char        tbuff [MAXPATHLEN];
  bool        exists;

  /* setrundir includes the macos prefix */
  installerSetrundir (installer, dir);

  snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4%s",
      installer->rundir, sysvarsGetStr (SV_OS_EXEC_EXT));
  exists = fileopFileExists (tbuff);

  if (exists) {
    installer->newinstall = false;
  } else {
    installer->newinstall = true;
  }

  return exists;
}

static void
installerSetPaths (installer_t *installer)
{
  /* the target dir should already be set by the validation process */
  installerSetBDJ3LocDir (installer, uiEntryGetValue (installer->bdj3locEntry));
  installerDisplayConvert (installer);
}

/* installation routines */

static void
installerVerifyInstInit (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Verifying installation."), false);
  installerDisplayText (installer, INST_DISP_STATUS, installer->pleasewaitmsg, false);
  uiLabelSetText (&installer->statusMsg, installer->pleasewaitmsg);

  /* the unpackdir is not necessarily the same as the current dir */
  /* on mac os, they are different */
  if (chdir (installer->unpackdir) < 0) {
    installerFailWorkingDir (installer, installer->unpackdir, "verifyinstinit");
    return;
  }

  installer->instState = INST_VERIFY_INSTALL;
}

static void
installerVerifyInstall (installer_t *installer)
{
  char        tmp [40];
  const char  *targv [2];

  if (isWindows ()) {
    if (installerWinVerifyProcess (installer)) {
      strlcpy (tmp, "OK", sizeof (tmp));
    } else {
      strlcpy (tmp, "NG", sizeof (tmp));
    }
  } else {
    targv [0] = "./install/verifychksum.sh";
    if (isMacOS ()) {
      targv [0] = "./Contents/MacOS/install/verifychksum.sh";
    }
    targv [1] = NULL;
    osProcessPipe (targv, OS_PROC_DETACH, tmp, sizeof (tmp), NULL);
  }

  uiLabelSetText (&installer->statusMsg, "");
  if (strncmp (tmp, "OK", 2) == 0) {
    /* CONTEXT: installer: status message */
    installerDisplayText (installer, INST_DISP_STATUS, _("Verification complete."), false);
    installer->instState = INST_PREPARE;
  } else {
    /* CONTEXT: installer: status message */
    installerDisplayText (installer, INST_DISP_ERROR, _("Verification failed."), false);
    installer->instState = INST_PREPARE;
  }
}

static void
installerInstInit (installer_t *installer)
{
  bool        exists = false;
  bool        found = false;
  char        tbuff [MAXPATHLEN];

  if (installer->guienabled) {
    installerSetPaths (installer);
  }

  if (installer->guienabled) {
    installer->reinstall = uiToggleButtonIsActive (&installer->reinstWidget);
    if (installer->newinstall) {
      installer->reinstall = false;
    }
  }

  if (! installer->guienabled && ! installer->unattended) {
    tbuff [0] = '\0';
    /* CONTEXT: installer: command line interface: asking for the BDJ4 destination */
    printf (_("Enter the destination folder."));
    printf ("\n");
    /* CONTEXT: installer: command line interface: instructions */
    printf (_("Press 'Enter' to select the default."));
    printf ("\n");
    printf ("[%s] : ", installer->target);
    fflush (stdout);
    (void) ! fgets (tbuff, sizeof (tbuff), stdin);
    stringTrim (tbuff);
    if (*tbuff != '\0') {
      dataFree (installer->target);
      installer->target = mdstrdup (tbuff);
    }
  }

  exists = fileopIsDirectory (installer->target);
  if (exists) {
    found = installerCheckTarget (installer, installer->target);

    if (found && ! installer->guienabled && ! installer->unattended) {
      printf ("\n");
      if (installer->reinstall) {
        /* CONTEXT: installer: command line interface: indicating action */
        printf (_("Re-install %s."), BDJ4_NAME);
      } else {
        /* CONTEXT: installer: command line interface: indicating action */
        printf (_("Updating existing %s installation."), BDJ4_NAME);
      }
      printf ("\n");
      fflush (stdout);

      /* CONTEXT: installer: command line interface: prompt to continue */
      printf (_("Proceed with installation?"));
      printf ("\n");
      printf ("[Y] : ");
      fflush (stdout);
      (void) ! fgets (tbuff, sizeof (tbuff), stdin);
      stringTrim (tbuff);
      if (*tbuff != '\0') {
        if (strncmp (tbuff, "Y", 1) != 0 &&
            strncmp (tbuff, "y", 1) != 0) {
          /* CONTEXT: installer: command line interface: status message */
          printf (" * %s", _("Installation aborted."));
          printf ("\n");
          fflush (stdout);
          installer->instState = INST_WAIT_USER;
          return;
        }
      }
    }

    if (! found) {
      /* CONTEXT: installer: command line interface: indicating action */
      printf (_("New %s installation."), BDJ4_NAME);

      /* do not allow an overwrite of an existing directory that is not bdj4 */
      if (installer->guienabled) {
        /* CONTEXT: installer: command line interface: status message */
        uiLabelSetText (&installer->feedbackMsg, _("Folder already exists."));
      }

      /* CONTEXT: installer: command line interface: the selected folder exists and is not a BDJ4 installation */
      snprintf (tbuff, sizeof (tbuff), _("Error: Folder %s already exists."),
          installer->target);
      installerDisplayText (installer, INST_DISP_ERROR, tbuff, false);
      /* CONTEXT: installer: command line interface: status message */
      installerDisplayText (installer, INST_DISP_ERROR, _("Installation aborted."), false);
      installer->instState = INST_WAIT_USER;
      return;
    }
  }

  installerSetrundir (installer, installer->target);
  installer->instState = INST_SAVE_TARGET;
}

static void
installerSaveTargetDir (installer_t *installer)
{
  char        tbuff [MAXPATHLEN];
  FILE        *fh;

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Saving install location."), false);

  installerGetTargetSaveFname (installer, tbuff, sizeof (tbuff));
  uiEntrySetValue (installer->targetEntry, installer->target);

  fh = fileopOpen (tbuff, "w");
  if (fh != NULL) {
    fprintf (fh, "%s\n", installer->target);
    fclose (fh);
  }

  installer->instState = INST_MAKE_TARGET;
}

static void
installerMakeTarget (installer_t *installer)
{
  char    tbuff [MAXPATHLEN];
  char    *data;
  char    *p;
  char    *tp;
  char    *tokptr;
  char    *tokptrb;

  diropMakeDir (installer->target);
  diropMakeDir (installer->rundir);

  *installer->oldversion = '\0';
  snprintf (tbuff, sizeof (tbuff), "%s/VERSION.txt",
      installer->target);
  if (fileopFileExists (tbuff)) {
    char *nm, *ver, *build, *bdate, *rlvl, *rdev;

    nm = "";
    ver = "";
    build = "";
    bdate = "";
    rlvl = "";
    rdev = "";
    data = filedataReadAll (tbuff, NULL);
    tp = strtok_r (data, "\r\n", &tokptr);
    while (tp != NULL) {
      nm = strtok_r (tp, "=", &tokptrb);
      p = strtok_r (NULL, "=", &tokptrb);
      if (nm != NULL && p != NULL && strcmp (nm, "VERSION") == 0) {
        ver = p;
      }
      if (nm != NULL && p != NULL && strcmp (nm, "BUILD") == 0) {
        build = p;
      }
      if (nm != NULL && p != NULL && strcmp (nm, "BUILDDATE") == 0) {
        bdate = p;
      }
      if (nm != NULL && p != NULL && strcmp (nm, "RELEASELEVEL") == 0) {
        rlvl = p;
      }
      if (nm != NULL && p != NULL && strcmp (nm, "DEVELOPMENT") == 0) {
        rdev = p;
      }
      if (p != NULL) {
        stringTrim (p);
      }
      tp = strtok_r (NULL, "\r\n", &tokptr);
    }
    strlcat (installer->oldversion, ver, sizeof (installer->oldversion));
    if (*rlvl) {
      strlcat (installer->oldversion, "-", sizeof (installer->oldversion));
      strlcat (installer->oldversion, rlvl, sizeof (installer->oldversion));
    }
    if (*rdev) {
      strlcat (installer->oldversion, "-", sizeof (installer->oldversion));
      strlcat (installer->oldversion, rdev, sizeof (installer->oldversion));
    }
    strlcat (installer->oldversion, "-", sizeof (installer->oldversion));
    strlcat (installer->oldversion, bdate, sizeof (installer->oldversion));
    strlcat (installer->oldversion, "-", sizeof (installer->oldversion));
    strlcat (installer->oldversion, build, sizeof (installer->oldversion));
    mdfree (data);
  }

  installer->instState = INST_COPY_START;
}

static void
installerCopyStart (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Copying files."), false);
  installerDisplayText (installer, INST_DISP_STATUS, installer->pleasewaitmsg, false);
  uiLabelSetText (&installer->statusMsg, installer->pleasewaitmsg);

  /* the unpackdir is not necessarily the same as the current dir */
  /* on mac os, they are different */
  if (chdir (installer->unpackdir) < 0) {
    installerFailWorkingDir (installer, installer->unpackdir, "copystart");
    return;
  }

  installer->instState = INST_COPY_FILES;
}

static void
installerCopyFiles (installer_t *installer)
{
  char      tbuff [MAXPATHLEN];
  char      tmp [MAXPATHLEN];

  if (isWindows ()) {
    strlcpy (tmp, installer->rundir, sizeof (tmp));
    pathWinPath (tmp, sizeof (tmp));
    snprintf (tbuff, sizeof (tbuff),
        "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl . \"%s\"",
        tmp);
  } else {
    snprintf (tbuff, sizeof (tbuff), "tar -c -f - . | (cd '%s'; tar -x -f -)",
        installer->target);
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "copy files: %s", tbuff);
  (void) ! system (tbuff);
  uiLabelSetText (&installer->statusMsg, "");
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_STATUS, _("Copy finished."), false);

  if (installer->nodatafiles) {
    installer->instState = INST_VLC_CHECK;
  } else {
    installer->instState = INST_MAKE_DATA_TOP;
  }
}

static void
installerMakeDataTop (installer_t *installer)
{
  strlcpy (installer->datatopdir, installer->rundir, MAXPATHLEN);
  if (isMacOS ()) {
    snprintf (installer->datatopdir, MAXPATHLEN,
        "%s/Library/Application Support/BDJ4",
        installer->home);
  }

  diropMakeDir (installer->datatopdir);

  if (chdir (installer->datatopdir)) {
    installerFailWorkingDir (installer, installer->datatopdir, "makedatatop");
    return;
  }

  installer->instState = INST_CREATE_DIRS;
}

static void
installerCreateDirs (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Creating folder structure."), false);

  /* this will create the directories necessary for the configs */
  bdjoptCreateDirectories ();
  /* create the directories that are not included in the distribution */
  diropMakeDir ("tmp");
  diropMakeDir ("img/profile00");

  installer->instState = INST_COPY_TEMPLATES_INIT;
}

static void
installerCopyTemplatesInit (installer_t *installer)
{
  if (! installer->newinstall && ! installer->reinstall) {
    installer->instState = INST_CONVERT_START;
    return;
  }

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Copying template files."), false);
  installer->instState = INST_COPY_TEMPLATES;
}

static void
installerCopyTemplates (installer_t *installer)
{
  char            dir [MAXPATHLEN];
  char            from [MAXPATHLEN];
  char            to [MAXPATHLEN];
  char            tbuff [MAXPATHLEN];
  const char      *fname;
  slist_t         *dirlist;
  slistidx_t      iteridx;
  pathinfo_t      *pi;
  datafile_t      *srdf;
  datafile_t      *qddf;
  datafile_t      *autodf;
  slist_t         *renamelist;


  if (chdir (installer->datatopdir)) {
    installerFailWorkingDir (installer, installer->datatopdir, "copytemplates");
    return;
  }

  renamelist = NULL;

  snprintf (tbuff, sizeof (tbuff), "%s/install/%s",
      installer->rundir, "localized-sr.txt");
  srdf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL, tbuff, NULL, 0);
  snprintf (tbuff, sizeof (tbuff), "%s/install/%s",
      installer->rundir, "localized-auto.txt");
  autodf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL, tbuff, NULL, 0);
  snprintf (tbuff, sizeof (tbuff), "%s/install/%s",
      installer->rundir, "localized-qd.txt");
  qddf = datafileAllocParse ("loc-qd", DFTYPE_KEY_VAL, tbuff, NULL, 0);

  snprintf (dir, sizeof (dir), "%s/templates", installer->rundir);

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

    if (strcmp (fname, "bdj-flex-dark.html") == 0) {
      snprintf (from, sizeof (from), "%s", fname);
      snprintf (to, sizeof (to), "http/bdj4remote.html");
      installerTemplateCopy (dir, from, to);
      continue;
    }
    if (strcmp (fname, "mobilemq.html") == 0) {
      snprintf (from, sizeof (from), "%s", fname);
      snprintf (to, sizeof (to), "http/%s", fname);
      installerTemplateCopy (dir, from, to);
      continue;
    }

    pi = pathInfo (fname);
    if (pathInfoExtCheck (pi, ".html")) {
      mdfree (pi);
      continue;
    }

    if (pathInfoExtCheck (pi, ".crt")) {
      snprintf (from, sizeof (from), "%s", fname);
      snprintf (to, sizeof (to), "http/%s", fname);
    } else if (strncmp (fname, "bdjconfig", 9) == 0) {
      snprintf (from, sizeof (from), "%s", fname);

      snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->blen, pi->basename);
      if (pathInfoExtCheck (pi, ".g")) {
        snprintf (to, sizeof (to), "data/%s", tbuff);
      } else if (pathInfoExtCheck (pi, ".p")) {
        snprintf (to, sizeof (to), "data/profile00/%s", tbuff);
      } else if (pathInfoExtCheck (pi, ".txt")) {
        snprintf (to, sizeof (to), "data/profile00/%s", fname);
      } else if (pathInfoExtCheck (pi, ".m")) {
        snprintf (to, sizeof (to), "data/%s/%s", installer->hostname, tbuff);
      } else if (pathInfoExtCheck (pi, ".mp")) {
        snprintf (to, sizeof (to), "data/%s/profile00/%s",
            installer->hostname, tbuff);
      } else {
        /* one of the localized versions */
        mdfree (pi);
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

      snprintf (from, sizeof (from), "%s", tbuff);
      if (strncmp (pi->basename, "ds-", 3) == 0) {
        snprintf (to, sizeof (to), "data/profile00/%s", tbuff);
      } else {
        snprintf (to, sizeof (to), "data/%s", tbuff);
      }
    } else {
      /* uknown extension, probably a localized file */
      mdfree (pi);
      continue;
    }

    installerTemplateCopy (dir, from, to);
    mdfree (pi);
  }
  slistFree (dirlist);

  snprintf (dir, sizeof (dir), "%s/templates/img", installer->rundir);
  dirlist = dirlistBasicDirList (dir, NULL);
  slistStartIterator (dirlist, &iteridx);
  while ((fname = slistIterateKey (dirlist, &iteridx)) != NULL) {
    snprintf (from, sizeof (from), "%s", fname);
    snprintf (to, sizeof (to), "img/profile00/%s", fname);
    installerTemplateCopy (dir, from, to);
  }
  slistFree (dirlist);

  snprintf (dir, sizeof (dir), "%s/img", installer->rundir);

  strlcpy (from, "led_on.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/led_on.svg");
  installerTemplateCopy (dir, from, to);

  strlcpy (from, "led_off.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/led_off.svg");
  installerTemplateCopy (dir, from, to);

  if (isMacOS ()) {
    snprintf (to, sizeof (to), "%s/.themes", installer->home);
    diropMakeDir (to);

    snprintf (from, sizeof (from), "../Applications/BDJ4.app/Contents/MacOS/plocal/share/themes/macOS-Mojave-dark");
    snprintf (to, sizeof (to), "%s/.themes/macOS-Mojave-dark", installer->home);
    filemanipLinkCopy (from, to);

    snprintf (from, sizeof (from), "../Applications/BDJ4.app/Contents/MacOS/plocal/share/themes/macOS-Mojave-light");
    snprintf (to, sizeof (to), "%s/.themes/macOS-Mojave-light", installer->home);
    filemanipLinkCopy (from, to);
  }

  datafileFree (srdf);
  datafileFree (autodf);
  datafileFree (qddf);

  installer->instState = INST_CONVERT_START;
}

/* conversion routines */

static void
installerConvertStart (installer_t *installer)
{
  char    tbuff [MAXPATHLEN];
  char    *locs [15];
  int     locidx = 0;
  char    *data;
  char    *p;
  char    *tp;
  char    *tokptr;
  char    *tokptrb;
  char    *vnm;
  int     ok;


  if (! installer->convprocess) {
    installer->instState = INST_CREATE_SHORTCUT;
    return;
  }

  if (strcmp (installer->bdj3loc, "-") == 0 ||
     *installer->bdj3loc == '\0') {
    installer->instState = INST_CREATE_SHORTCUT;
    return;
  }

  if (! installer->guienabled && ! installer->unattended) {
    tbuff [0] = '\0';
    /* CONTEXT: installer: command line interface: prompt for BDJ3 location */
    printf (_("Enter the folder where %s is installed."), BDJ3_NAME);
    printf ("\n");
    /* CONTEXT: installer: command line interface: instructions */
    printf (_("The conversion process will only run for new installations and for re-installs."));
    printf ("\n");
    /* CONTEXT: installer: command line interface: accept BDJ3 location default */
    printf (_("Press 'Enter' to select the default."));
    printf ("\n");
    /* CONTEXT: installer: command line interface: instructions */
    printf (_("If there is no %s installation, enter a single '-'."), BDJ3_NAME);
    printf ("\n");
    /* CONTEXT: installer: command line interface: BDJ3 location prompt */
    snprintf (tbuff, sizeof (tbuff), _("%s Folder"), BDJ3_NAME);
    printf ("%s [%s] : ", tbuff, installer->bdj3loc);
    fflush (stdout);
    (void) ! fgets (tbuff, sizeof (tbuff), stdin);
    stringTrim (tbuff);
    if (*tbuff != '\0') {
      installerSetBDJ3LocDir (installer, tbuff);
    }
  }

  if (strcmp (installer->bdj3loc, "-") == 0 ||
     *installer->bdj3loc == '\0') {
    installer->instState = INST_CREATE_SHORTCUT;
    return;
  }

  if (chdir (installer->rundir)) {
    installerFailWorkingDir (installer, installer->rundir, "convertstart");
    return;
  }

  installerDisplayText (installer, INST_DISP_ACTION,
      /* CONTEXT: installer: status message */
      _("Starting conversion process."), false);

  ok = false;
  snprintf (tbuff, sizeof (tbuff), "%s/data/musicdb.txt", installer->bdj3loc);
  if (! fileopFileExists (tbuff)) {
    /* try the old database name */
    snprintf (tbuff, sizeof (tbuff), "%s/data/masterlist.txt", installer->bdj3loc);
  }
  if (! fileopFileExists (tbuff)) {
    /* macos path */
    snprintf (tbuff, sizeof (tbuff), "%s/Library/Application Support/BallroomDJ/data/musicdb.txt", installer->home);
  }
  if (! fileopFileExists (tbuff)) {
    /* macos path, old database name */
    snprintf (tbuff, sizeof (tbuff), "%s/Library/Application Support/BallroomDJ/data/masterlist.txt", installer->home);
  }

  if (fileopFileExists (tbuff)) {
    FILE  *fh;
    char  tmp [200];
    int   ver;

    fh = fileopOpen (tbuff, "r");
    if (fh != NULL) {
      (void) ! fgets (tmp, sizeof (tmp), fh);
      fclose (fh);
      sscanf (tmp, "#VERSION=%d", &ver);
      if (ver < 7) {
        /* CONTEXT: installer: status message */
        installerDisplayText (installer, INST_DISP_ERROR, _("BDJ3 database version is too old."), true);
      } else {
        ok = true;
      }
    } else {
      installerDisplayText (installer, INST_DISP_ERROR,
          /* CONTEXT: installer: status message */
          _("Unable to locate BDJ3 database."), true);
    }
  }

  if (! ok) {
    installer->instState = INST_CREATE_SHORTCUT;
    return;
  }

  *installer->bdj3version = '\0';
  snprintf (tbuff, sizeof (tbuff), "%s/VERSION.txt",
      installer->bdj3loc);
  if (isMacOS () && ! fileopFileExists (tbuff)) {
    snprintf (tbuff, sizeof (tbuff),
        "%s/Applications/BallroomDJ.app%s/VERSION.txt",
        installer->home, MACOS_PREFIX);
  }
  if (fileopFileExists (tbuff)) {
    data = filedataReadAll (tbuff, NULL);
    tp = strtok_r (data, "\r\n", &tokptr);
    while (tp != NULL) {
      vnm = strtok_r (tp, "=", &tokptrb);
      p = strtok_r (NULL, "=", &tokptrb);
      if (strcmp (vnm, "VERSION") == 0) {
        strlcat (installer->bdj3version, p, sizeof (installer->bdj3version));
        stringTrim (installer->bdj3version);
        break;
      }
      tp = strtok_r (NULL, "\r\n", &tokptr);
    }
    mdfree (data);
  }

  installer->convlist = dirlistBasicDirList ("conv", ".tcl");
  /* the sort order doesn't matter, but there's a need to run */
  /* a check after everything has finished to make sure the user's */
  /* organization path is in orgopt.txt */
  /* having a sorted list makes it easy to name something to run last */
  slistSort (installer->convlist);
  slistStartIterator (installer->convlist, &installer->convidx);

  locidx = 0;
  snprintf (tbuff, sizeof (tbuff), "%s/%s/%"PRId64"/tcl/bin/tclsh",
      installer->bdj3loc, sysvarsGetStr (SV_OSNAME), sysvarsGetNum (SVL_OSBITS));
  locs [locidx++] = mdstrdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/Applications/BallroomDJ.app/Contents/%s/%"PRId64"/tcl/bin/tclsh",
      installer->home, sysvarsGetStr (SV_OSNAME), sysvarsGetNum (SVL_OSBITS));
  locs [locidx++] = mdstrdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/local/bin/tclsh", installer->home);
  locs [locidx++] = mdstrdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/bin/tclsh", installer->home);
  locs [locidx++] = mdstrdup (tbuff);
  /* for testing; low priority */
  snprintf (tbuff, sizeof (tbuff), "%s/../%s/%"PRId64"/tcl/bin/tclsh",
      installer->bdj3loc, sysvarsGetStr (SV_OSNAME), sysvarsGetNum (SVL_OSBITS));
  locs [locidx++] = mdstrdup (tbuff);
  locs [locidx++] = mdstrdup ("/opt/local/bin/tclsh");
  locs [locidx++] = mdstrdup ("/usr/bin/tclsh");
  locs [locidx++] = NULL;

  locidx = 0;
  while (locs [locidx] != NULL) {
    strlcpy (tbuff, locs [locidx], sizeof (tbuff));
    if (isWindows ()) {
      snprintf (tbuff, sizeof (tbuff), "%s.exe", locs [locidx]);
    }

    if (installer->tclshloc == NULL && fileopFileExists (tbuff)) {
      if (isWindows ()) {
        pathWinPath (tbuff, sizeof (tbuff));
      }
      installer->tclshloc = mdstrdup (tbuff);
      /* CONTEXT: installer: status message */
      installerDisplayText (installer, INST_DISP_STATUS, _("Located 'tclsh'."), false);
    }

    mdfree (locs [locidx]);
    ++locidx;
  }

  if (installer->tclshloc == NULL) {
    /* CONTEXT: installer: failure message */
    snprintf (tbuff, sizeof (tbuff), _("Unable to locate %s."), "tclsh");
    installerDisplayText (installer, INST_DISP_STATUS, tbuff, false);
    /* CONTEXT: installer: status message */
    installerDisplayText (installer, INST_DISP_STATUS, _("Skipping conversion."), false);
    installer->instState = INST_CREATE_SHORTCUT;
    return;
  }

  installer->instState = INST_CONVERT;
}

static void
installerConvert (installer_t *installer)
{
  char      *fn;
  char      buffa [MAXPATHLEN];
  char      buffb [MAXPATHLEN];
  const char *targv [15];

  fn = slistIterateKey (installer->convlist, &installer->convidx);
  if (fn == NULL) {
    installer->instState = INST_CONVERT_FINISH;
    return;
  }

  /* CONTEXT: installer: status message */
  snprintf (buffa, sizeof (buffa), _("Running conversion script: %s."), fn);
  installerDisplayText (installer, INST_DISP_STATUS, buffa, false);

  targv [0] = installer->tclshloc;
  snprintf (buffa, sizeof (buffa), "conv/%s", fn);
  targv [1] = buffa;
  snprintf (buffb, sizeof (buffb), "%s/data", installer->bdj3loc);
  targv [2] = buffb;
  targv [3] = installer->datatopdir;
  targv [4] = NULL;

  osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);

  installer->instState = INST_CONVERT;
  return;
}

static void
installerConvertFinish (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_STATUS, _("Conversion complete."), false);
  installer->instState = INST_CREATE_SHORTCUT;
}

static void
installerCreateShortcut (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];

  if (chdir (installer->rundir)) {
    installerFailWorkingDir (installer, installer->rundir, "createshortcut");
    return;
  }

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Creating shortcut."), false);

  /* pathbld_mp_dir_exec is used by instutilCreateShortcut() */
  snprintf (tbuff, sizeof (tbuff), "%s/bin", installer->target);
  sysvarsSetStr (SV_BDJ4_DIR_EXEC, tbuff);
  /* handles linux and windows */
  instutilCreateShortcut (BDJ4_NAME, installer->rundir, installer->rundir, 0);

  if (isMacOS ()) {
#if _lib_symlink
    char        buff [MAXPATHLEN];

    /* on macos, the startup program must be a gui program, otherwise */
    /* the dock icon is not correct */
    /* this must exist and match the name of the app */
    (void) ! symlink ("bin/bdj4g", "BDJ4");
    /* desktop shortcut */
    snprintf (buff, sizeof (buff), "%s/Desktop/BDJ4.app", installer->home);
    (void) ! symlink (installer->target, buff);
#endif
  }

  installer->instState = INST_VLC_CHECK;
}

static void
installerVLCCheck (installer_t *installer)
{
  char    tbuff [MAXPATHLEN];

  /* on linux, vlc is installed via other methods */
  if (installer->vlcinstalled || isLinux ()) {
    installer->instState = INST_PYTHON_CHECK;
    return;
  }

  installerVLCGetVersion (installer);
  if (*installer->vlcversion) {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Downloading %s."), "VLC");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installer->instState = INST_VLC_DOWNLOAD;
  } else {
    snprintf (tbuff, sizeof (tbuff),
        /* CONTEXT: installer: status message */
        _("Unable to determine %s version."), "VLC");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installer->instState = INST_PYTHON_CHECK;
  }
}

static void
installerVLCDownload (installer_t *installer)
{
  char  url [MAXPATHLEN];
  char  tbuff [MAXPATHLEN];

  *url = '\0';
  *installer->dlfname = '\0';
  if (isWindows ()) {
    snprintf (installer->dlfname, sizeof (installer->dlfname),
        "vlc-%s-win%"PRId64".exe",
        installer->vlcversion, sysvarsGetNum (SVL_OSBITS));
    snprintf (url, sizeof (url),
        "https://get.videolan.org/vlc/last/win%"PRId64"/%s",
        sysvarsGetNum (SVL_OSBITS), installer->dlfname);
  }
  if (isMacOS ()) {
    char    *arch;

    /* the os architecture on m1 is 'arm64' and includes the bits */
    /* the os architecture on intel is 'x86_64' */
    arch = sysvarsGetStr (SV_OSARCH);
    if (strcmp (arch, "x86_64") == 0) {
      arch = "intel64";
    }
    snprintf (installer->dlfname, sizeof (installer->dlfname),
        "vlc-%s-%s.dmg", installer->vlcversion, arch);
    snprintf (url, sizeof (url),
        "https://get.videolan.org/vlc/last/macosx/%s",
        installer->dlfname);
  }
  if (*url && *installer->vlcversion) {
    webclientDownload (installer->webclient, url, installer->dlfname);
  }

  if (fileopFileExists (installer->dlfname)) {
    chmod (installer->dlfname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#if _lib_removexattr
    removexattr (installer->dlfname, "com.apple.quarantine", XATTR_NOFOLLOW);
#endif
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Installing %s."), "VLC");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installerDisplayText (installer, INST_DISP_STATUS, installer->pleasewaitmsg, false);
    uiLabelSetText (&installer->statusMsg, installer->pleasewaitmsg);
    installer->instState = INST_VLC_INSTALL;
  } else {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Download of %s failed."), "VLC");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installer->instState = INST_PYTHON_CHECK;
  }
}

static void
installerVLCInstall (installer_t *installer)
{
  char    tbuff [MAXPATHLEN];

  if (fileopFileExists (installer->dlfname)) {
    if (isWindows ()) {
      snprintf (tbuff, sizeof (tbuff), ".\\%s", installer->dlfname);
    }
    if (isMacOS ()) {
      snprintf (tbuff, sizeof (tbuff), "open ./%s", installer->dlfname);
    }
    /* both windows and macos need to run this via the system call */
    /* due to privilege escalation */
    (void) ! system (tbuff);
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("%s installed."), "VLC");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installer->vlcinstalled = true;
  }
  fileopDelete (installer->dlfname);
  uiLabelSetText (&installer->statusMsg, "");
  installerCheckPackages (installer);
  installer->instState = INST_PYTHON_CHECK;
}

static void
installerPythonCheck (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];

  /* python is installed via other methods on linux and macos */
  if (isLinux () || isMacOS ()) {
    installer->instState = INST_MUTAGEN_CHECK;
    return;
  }

  installerPythonGetVersion (installer);

  if (installer->pythoninstalled) {
    char    *p;
    int     majvers = 0;
    int     minvers = 0;

    strlcpy (tbuff, installer->pyversion, sizeof (tbuff));
    majvers = atoi (installer->pyversion);
    p = strstr (tbuff, ".");
    if (p != NULL) {
      minvers = atoi (p + 1);
      minvers -= 3;
    }
    snprintf (tbuff, sizeof (tbuff), "%d.%d", majvers, minvers);

    /* check for old versions of python and update */
    /* does this conflict w/bdj3? */
    if (versionCompare (sysvarsGetStr (SV_PYTHON_DOT_VERSION), tbuff) < 0) {
      installer->updatepython = true;
    }
  }

  if (installer->pythoninstalled && ! installer->updatepython) {
    installer->instState = INST_MUTAGEN_CHECK;
    return;
  }

  if (*installer->pyversion) {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Downloading %s."), "Python");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installer->instState = INST_PYTHON_DOWNLOAD;
  } else {
    snprintf (tbuff, sizeof (tbuff),
        /* CONTEXT: installer: status message */
        _("Unable to determine %s version."), "Python");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installer->instState = INST_MUTAGEN_CHECK;
  }
}

static void
installerPythonDownload (installer_t *installer)
{
  char  url [MAXPATHLEN];
  char  tbuff [MAXPATHLEN];

  *url = '\0';
  *installer->dlfname = '\0';
  if (isWindows ()) {
    char  *tag;

    /* https://www.python.org/ftp/python/3.10.2/python-3.10.2.exe */
    /* https://www.python.org/ftp/python/3.10.2/python-3.10.2-amd64.exe */
    tag = "";
    if (sysvarsGetNum (SVL_OSBITS) == 64) {
      tag = "-amd64";
    }
    snprintf (installer->dlfname, sizeof (installer->dlfname),
        "python-%s%s.exe",
        installer->pyversion, tag);
    snprintf (url, sizeof (url),
        "https://www.python.org/ftp/python/%s/%s",
        installer->pyversion, installer->dlfname);
  }
  if (*url && *installer->pyversion) {
    webclientDownload (installer->webclient, url, installer->dlfname);
  }

  if (fileopFileExists (installer->dlfname)) {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Installing %s."), "Python");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installerDisplayText (installer, INST_DISP_STATUS, installer->pleasewaitmsg, false);
    uiLabelSetText (&installer->statusMsg, installer->pleasewaitmsg);
    installer->instState = INST_PYTHON_INSTALL;
  } else {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Download of %s failed."), "Python");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installer->instState = INST_MUTAGEN_CHECK;
  }
}

static void
installerPythonInstall (installer_t *installer)
{
  char        tbuff [MAXPATHLEN];
  int         targc = 0;
  const char  *targv [20];

  if (fileopFileExists (installer->dlfname)) {
    snprintf (tbuff, sizeof (tbuff), ".\\%s", installer->dlfname);
    targv [targc++] = tbuff;
    targv [targc++] = "/quiet";
    //targv [targc++] = "/passive";
    targv [targc++] = "InstallAllUsers=0";
    targv [targc++] = "Shortcuts=0";
    targv [targc++] = "CompileAll=1";
    targv [targc++] = "PrependPath=1";
    targv [targc++] = "Include_doc=0";
    targv [targc++] = "Include_launcher=0";
    targv [targc++] = "InstallLauncherAllUsers=0";
    targv [targc++] = "Include_tcltk=0";
    targv [targc++] = "Include_test=0";
    targv [targc++] = NULL;
    osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("%s installed."), "Python");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installer->pythoninstalled = true;
  }
  fileopDelete (installer->dlfname);
  uiLabelSetText (&installer->statusMsg, "");
  installerCheckPackages (installer);
  installer->instState = INST_MUTAGEN_CHECK;
}

static void
installerMutagenCheck (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];

  if (installer->nodatafiles) {
    installer->instState = INST_FINALIZE;
    return;
  }

  if (! installer->pythoninstalled) {
    installer->instState = INST_UPDATE_PROCESS_INIT;
    return;
  }

  /* CONTEXT: installer: status message */
  snprintf (tbuff, sizeof (tbuff), _("Installing %s."), "Mutagen");
  installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
  installerDisplayText (installer, INST_DISP_STATUS, installer->pleasewaitmsg, false);
  uiLabelSetText (&installer->statusMsg, installer->pleasewaitmsg);
  installer->instState = INST_MUTAGEN_INSTALL;
}

static void
installerMutagenInstall (installer_t *installer)
{
  char      tbuff [MAXPATHLEN];
  char      *pipnm = "pip";

  if (installer->pythoninstalled) {
    char  *tptr;

    tptr = sysvarsGetStr (SV_PATH_PYTHON_PIP);
    if (tptr != NULL && *tptr) {
      pipnm = tptr;
    }
  }
  snprintf (tbuff, sizeof (tbuff),
      "%s --quiet install --user --upgrade mutagen", pipnm);
  (void) ! system (tbuff);
  uiLabelSetText (&installer->statusMsg, "");
  /* CONTEXT: installer: status message */
  snprintf (tbuff, sizeof (tbuff), _("%s installed."), "Mutagen");
  installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
  installerCheckPackages (installer);
  installer->instState = INST_UPDATE_PROCESS_INIT;
}

static void
installerUpdateProcessInit (installer_t *installer)
{
  char  buff [MAXPATHLEN];

  /* CONTEXT: installer: status message */
  snprintf (buff, sizeof (buff), _("Updating %s."), BDJ4_LONG_NAME);
  installerDisplayText (installer, INST_DISP_ACTION, buff, false);
  installerDisplayText (installer, INST_DISP_STATUS, installer->pleasewaitmsg, false);
  uiLabelSetText (&installer->statusMsg, installer->pleasewaitmsg);
  installer->instState = INST_UPDATE_PROCESS;
}

static void
installerUpdateProcess (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];
  int   targc = 0;
  const char  *targv [10];

  snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4%s",
      installer->rundir, sysvarsGetStr (SV_OS_EXEC_EXT));
  targv [targc++] = tbuff;
  targv [targc++] = "--wait";
  targv [targc++] = "--bdj4updater";
  /* only need to run the 'newinstall' update process when the template */
  /* files have been copied */
  if (installer->newinstall || installer->reinstall) {
    targv [targc++] = "--newinstall";
  }
  if (installer->convprocess) {
    targv [targc++] = "--converted";
  }
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
  uiLabelSetText (&installer->statusMsg, "");

  installer->instState = INST_FINALIZE;
}

static void
installerFinalize (installer_t *installer)
{
  if (installer->verbose) {
    fprintf (stdout, "finish OK\n");
    if (*installer->bdj3version) {
      fprintf (stdout, "bdj3-version %s\n", installer->bdj3version);
    } else {
      fprintf (stdout, "bdj3-version x\n");
    }
    if (*installer->oldversion) {
      fprintf (stdout, "old-version %s\n", installer->oldversion);
    } else {
      fprintf (stdout, "old-version x\n");
    }
    fprintf (stdout, "new-install %d\n", installer->newinstall);
    fprintf (stdout, "re-install %d\n", installer->reinstall);
    fprintf (stdout, "update %d\n", ! installer->newinstall && ! installer->reinstall);
    fprintf (stdout, "converted %d\n", installer->convprocess);
  }

  if (! installer->nodatafiles) {
    /* create the new install flag file on a new install */
    if (installer->newinstall) {
      FILE  *fh;

      fh = fileopOpen (INST_NEW_FILE, "w");
      fclose (fh);
    }
  }

  if (installer->nodatafiles) {
    installer->instState = INST_FINISH;
  } else {
    installer->instState = INST_REGISTER_INIT;
  }
}

static void
installerRegisterInit (installer_t *installer)
{
  char    tbuff [200];

  if (strcmp (sysvarsGetStr (SV_USER), "bll") == 0 &&
      ! installer->testregistration) {
    /* no need to register */
    snprintf (tbuff, sizeof (tbuff), "Registration Skipped.");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installer->instState = INST_FINISH;
  } else {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Registering %s."), BDJ4_NAME);
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installer->instState = INST_REGISTER;
  }
}

static void
installerRegister (installer_t *installer)
{
  char          uri [200];
  char          tbuff [2048];

  installer->webresponse = NULL;
  installer->webresplen = 0;
  if (installer->webclient == NULL) {
    installer->webclient = webclientAlloc (installer, installerWebResponseCallback);
  }
  snprintf (uri, sizeof (uri), "%s/%s",
      sysvarsGetStr (SV_HOST_SUPPORTMSG), sysvarsGetStr (SV_URI_REGISTER));

  snprintf (tbuff, sizeof (tbuff),
      "key=%s"
      "&version=%s&build=%s&builddate=%s&releaselevel=%s"
      "&osname=%s&osdisp=%s&osvers=%s&osbuild=%s"
      "&pythonvers=%s"
      "&user=%s&host=%s"
      "&systemlocale=%s&locale=%s"
      "&bdj3version=%s&oldversion=%s"
      "&new=%d&reinstall=%d&update=%d&convert=%d",
      "9873453",  // key
      sysvarsGetStr (SV_BDJ4_VERSION),
      sysvarsGetStr (SV_BDJ4_BUILD),
      sysvarsGetStr (SV_BDJ4_BUILDDATE),
      sysvarsGetStr (SV_BDJ4_RELEASELEVEL),
      sysvarsGetStr (SV_OSNAME),
      sysvarsGetStr (SV_OSDISP),
      sysvarsGetStr (SV_OSVERS),
      sysvarsGetStr (SV_OSBUILD),
      sysvarsGetStr (SV_PYTHON_DOT_VERSION),
      sysvarsGetStr (SV_USER),
      sysvarsGetStr (SV_HOSTNAME),
      sysvarsGetStr (SV_LOCALE_SYSTEM),
      sysvarsGetStr (SV_LOCALE),
      installer->bdj3version,
      installer->oldversion,
      installer->newinstall,
      installer->reinstall,
      ! installer->newinstall && ! installer->reinstall,
      installer->convprocess
      );
  webclientPost (installer->webclient, uri, tbuff);

  installer->instState = INST_FINISH;
}

/* support routines */

static void
installerCleanup (installer_t *installer)
{
  char  buff [MAXPATHLEN];
  const char  *targv [10];

  if (installer->guienabled) {
    uiEntryFree (installer->targetEntry);
    uiEntryFree (installer->bdj3locEntry);
    uiTextBoxFree (installer->disptb);
    for (int i = 0; i < INST_BUTTON_MAX; ++i) {
      uiButtonFree (installer->buttons [i]);
    }
  }
  for (int i = 0; i < INST_CB_MAX; ++i) {
    callbackFree (installer->callbacks [i]);
  }
  dataFree (installer->target);
  dataFree (installer->bdj3loc);
  slistFree (installer->convlist);
  dataFree (installer->tclshloc);

  webclientClose (installer->webclient);

  if (! fileopIsDirectory (installer->unpackdir)) {
    return;
  }

  if (isWindows ()) {
    targv [0] = ".\\install\\install-rminstdir.bat";
    snprintf (buff, sizeof (buff), "\"%s\"", installer->unpackdir);
    targv [1] = buff;
    targv [2] = NULL;
    osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);
  } else {
    snprintf (buff, sizeof(buff), "rm -rf %s", installer->unpackdir);
    (void) ! system (buff);
  }
}

static void
installerDisplayText (installer_t *installer, const char *pfx,
    const char *txt, bool bold)
{
  if (installer->guienabled) {
    if (bold) {
      uiTextBoxAppendBoldStr (installer->disptb, pfx);
      uiTextBoxAppendBoldStr (installer->disptb, txt);
      uiTextBoxAppendStr (installer->disptb, "\n");
    } else {
      uiTextBoxAppendStr (installer->disptb, pfx);
      uiTextBoxAppendStr (installer->disptb, txt);
      uiTextBoxAppendStr (installer->disptb, "\n");
    }
    installer->scrolltoend = true;
  } else {
    if (! installer->quiet) {
      fprintf (stdout, "%s%s\n", pfx, txt);
      fflush (stdout);
    }
  }
}

static void
installerGetTargetSaveFname (installer_t *installer, char *buff, size_t sz)
{
  diropMakeDir (sysvarsGetStr (SV_DIR_CONFIG));
  snprintf (buff, sz, "%s/%s", sysvarsGetStr (SV_DIR_CONFIG), INST_SAVE_FNAME);
}

static void
installerGetBDJ3Fname (installer_t *installer, char *buff, size_t sz)
{
  *buff = '\0';
  if (*installer->target) {
    if (isMacOS ()) {
      snprintf (buff, sz, "%s/Contents/MacOS/install/%s",
          installer->target, BDJ3_LOC_FILE);
    } else {
      snprintf (buff, sz, "%s/install/%s",
          installer->target, BDJ3_LOC_FILE);
    }
  }
}

static void
installerTemplateCopy (const char *dir, const char *from, const char *to)
{
  char      *localetmpldir;
  char      tbuff [MAXPATHLEN];

  localetmpldir = sysvarsGetStr (SV_LOCALE);
  snprintf (tbuff, sizeof (tbuff), "%s/%s/%s",
      dir, localetmpldir, from);
  if (fileopFileExists (tbuff)) {
    logMsg (LOG_INSTALL, LOG_MAIN, "found: %s", tbuff);
    from = tbuff;
  } else {
    localetmpldir = sysvarsGetStr (SV_LOCALE_SHORT);
    snprintf (tbuff, sizeof (tbuff), "%s/%s/%s",
        dir, localetmpldir, from);
    if (fileopFileExists (tbuff)) {
      logMsg (LOG_INSTALL, LOG_MAIN, "found: %s", tbuff);
      from = tbuff;
    } else {
      snprintf (tbuff, sizeof (tbuff), "%s/%s", dir, from);
      if (fileopFileExists (tbuff)) {
        logMsg (LOG_INSTALL, LOG_MAIN, "found: %s", tbuff);
        from = tbuff;
      } else {
        logMsg (LOG_INSTALL, LOG_MAIN, "   ERR: not found");
        return;
      }
    }
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "- copy: %s", from);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "    to: %s", to);
  filemanipBackup (to, 1);
  filemanipCopy (from, to);
}

static void
installerSetrundir (installer_t *installer, const char *dir)
{
  installer->rundir [0] = '\0';
  if (*dir) {
    strlcpy (installer->rundir, dir, sizeof (installer->rundir));
    if (isMacOS ()) {
      strlcat (installer->rundir, MACOS_PREFIX, sizeof (installer->rundir));
    }
    pathNormPath (installer->rundir, sizeof (installer->rundir));
  }
}

static void
installerVLCGetVersion (installer_t *installer)
{
  char      *p;
  char      *e;
  char      *platform;
  char      tbuff [MAXPATHLEN];

  *installer->vlcversion = '\0';
  installer->webresponse = NULL;
  if (installer->webclient == NULL) {
    installer->webclient = webclientAlloc (installer, installerWebResponseCallback);
  }

  /* linux is not installed via this method, nor does it have a  */
  /* directory in the videolan 'last' directory */

  platform = "macosx";
  if (isWindows ()) {
    platform = "win64";
  }

  snprintf (tbuff, sizeof (tbuff), "https://get.videolan.org/vlc/last/%s/", platform);

  webclientGet (installer->webclient, tbuff);

  if (installer->webresponse != NULL) {
    char *srchvlc = "vlc-";
    /* note that all files excepting ".." start with vlc-<version> */
    /* vlc-<version>.<arch>.dmg{|.asc|.md5|.sha1|.sha256} */
    /* vlc-<version>.<arch>.{7z|exe|msi|zip|debugsys}{|.asc|.md5|.sha1|.sha256} */
    /* vlc-3.0.16-intel64.dmg */
    /* vlc-3.0.17.4-universal.dmg */
    /* vlc-3.0.17.4-win64.exe */
    p = strstr (installer->webresponse, srchvlc);
    if (p != NULL) {
      p += strlen (srchvlc);
      e = strstr (p, "-");
      strlcpy (installer->vlcversion, p, e - p + 1);
    }
  }
}

static void
installerPythonGetVersion (installer_t *installer)
{
  char      *p;
  char      *e;

  *installer->pyversion = '\0';
  installer->webresponse = NULL;
  if (installer->webclient == NULL) {
    installer->webclient = webclientAlloc (installer, installerWebResponseCallback);
  }
  webclientGet (installer->webclient, "https://www.python.org/downloads/windows/");

  if (installer->webresponse != NULL) {
    char *srchpy = "Release - Python ";
    p = strstr (installer->webresponse, srchpy);
    if (p != NULL) {
      p += strlen (srchpy);
      e = strstr (p, "<");
      strlcpy (installer->pyversion, p, e - p + 1);
    }
  }
}

static void
installerCheckPackages (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];
  char  *tmp;
  char  pypath [MAXPATHLEN];


  tmp = sysvarsGetStr (SV_PATH_PYTHON);
  *pypath = '\0';
  if (! *tmp && *installer->pyversion) {
    char tver [40];
    int  tverlen;
    int  dotflag;

    tverlen = 0;
    dotflag = 0;
    for (size_t i = 0; i < strlen (installer->pyversion); ++i) {
      if (installer->pyversion [i] == '.') {
        if (dotflag) {
          break;
        }
        dotflag = 1;
        continue;
      }
      tver [tverlen] = installer->pyversion [i];
      ++tverlen;
    }
    tver [tverlen] = '\0';
    snprintf (pypath, sizeof (pypath),
        "%s/AppData/Local/Programs/Python/Python%s;"
        "%s/AppData/Local/Programs/Python/Python%s/Scripts",
        installer->home, tver,
        installer->home, tver);
  }
  sysvarsCheckPaths (pypath);
  sysvarsGetPythonVersion ();
  sysvarsCheckMutagen ();

  tmp = sysvarsGetStr (SV_PATH_VLC);

  if (*tmp) {
    if (installer->uiBuilt) {
      /* CONTEXT: installer: display of package status */
      snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "VLC");
      uiLabelSetText (&installer->vlcMsg, tbuff);
    }
    installer->vlcinstalled = true;
  } else {
    if (installer->uiBuilt) {
      /* CONTEXT: installer: display of package status */
      snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "VLC");
      uiLabelSetText (&installer->vlcMsg, tbuff);
    }
    installer->vlcinstalled = false;
  }

  tmp = sysvarsGetStr (SV_PATH_PYTHON);

  if (*tmp) {
    if (installer->uiBuilt) {
      /* CONTEXT: installer: display of package status */
      snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "Python");
      uiLabelSetText (&installer->pythonMsg, tbuff);
    }
    installer->pythoninstalled = true;
  } else {
    if (installer->uiBuilt) {
      /* CONTEXT: installer: display of package status */
      snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Python");
      uiLabelSetText (&installer->pythonMsg, tbuff);
      /* CONTEXT: installer: display of package status */
      snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Mutagen");
      uiLabelSetText (&installer->mutagenMsg, tbuff);
    }
    installer->pythoninstalled = false;
  }

  if (installer->pythoninstalled) {
    tmp = sysvarsGetStr (SV_PYTHON_MUTAGEN);
    if (installer->uiBuilt) {
      if (*tmp) {
        /* CONTEXT: installer: display of package status */
        snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "Mutagen");
        uiLabelSetText (&installer->mutagenMsg, tbuff);
      } else {
        /* CONTEXT: installer: display of package status */
        snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Mutagen");
        uiLabelSetText (&installer->mutagenMsg, tbuff);
      }
    }
  }
}

static void
installerWebResponseCallback (void *userdata, char *resp, size_t len)
{
  installer_t *installer = userdata;

  installer->webresponse = resp;
  installer->webresplen = len;
  return;
}

static void
installerFailWorkingDir (installer_t *installer, const char *dir, const char *msg)
{
  fprintf (stderr, "Unable to set working dir: %s (%s)\n", dir, msg);
  /* CONTEXT: installer: failure message */
  installerDisplayText (installer, INST_DISP_ERROR, _("Error: Unable to set working folder."), false);
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ERROR, _("Installation aborted."), false);
  installer->instState = INST_WAIT_USER;
}

static void
installerSetTargetDir (installer_t *installer, const char *fn)
{
  char  *tmp;

  /* fn may be pointing to an allocated value, which is installer->target */
  /* bad code */
  tmp = mdstrdup (fn);
  dataFree (installer->target);
  installer->target = tmp;
  pathNormPath (installer->target, strlen (installer->target));
}

static void
installerSetBDJ3LocDir (installer_t *installer, const char *fn)
{
  dataFree (installer->bdj3loc);
  installer->bdj3loc = mdstrdup (fn);
  pathNormPath (installer->bdj3loc, strlen (installer->bdj3loc));
}

static void
installerCheckAndFixTarget (char *buff, size_t sz)
{
  pathinfo_t  *pi;
  const char  *nm;
  int         rc;

  pi = pathInfo (buff);
  nm = BDJ4_NAME;
  if (isMacOS ()) {
    nm = BDJ4_MACOS_DIR;
  }

  rc = strncmp (pi->filename, nm, pi->flen) == 0 &&
      pi->flen == strlen (nm);
  if (! rc) {
    stringTrimChar (buff, '/');
    strlcat (buff, "/", sz);
    strlcat (buff, nm, sz);
  }
  pathInfoFree (pi);
}

/* not in use */
/* windows verification is too slow */
static bool
installerWinVerifyProcess (installer_t *installer)
{
  FILE        *fh;
  char        tbuff [1024];
  char        tmp [1024];
  int         rc = true;
  const char  *targv [10];
  int         targc = 0;
  char        *p;
  char        *fn;
  char        *chksum;
  char        *tokstr;
  int         fnidx;

  if (! chdir ("bdj4-install")) {
    return false;
  }

  fh = fileopOpen ("install/checksum.txt", "r");
  if (fh == NULL) {
    return false;
  }

  /* the build process would need to leave openssl.exe intact */
  /* see also cleanuplist.txt */
  targv [targc++] = ".\\plocal\\bin\\openssl.exe";
  targv [targc++] = "sha512";
  targv [targc++] = "-r";
  fnidx = targc;
  targv [targc++] = NULL;
  targv [targc++] = NULL;

  while (fgets (tbuff, sizeof (tbuff), fh) != NULL) {
    p = strtok_r (tbuff, " ", &tokstr);
    chksum = p;
    p = strtok_r (NULL, " ", &tokstr);
    if (*p == '*') {
      ++p;
    }
    fn = p;
    stringTrim (fn);

    targv [fnidx] = fn;
    osProcessPipe (targv, OS_PROC_WAIT, tmp, sizeof (tmp), NULL);
    p = strtok_r (tmp, " ", &tokstr);
    if (strcmp (p, chksum) != 0) {
      rc = false;
      break;
    }
  }
  fclose (fh);

  return rc;
}
