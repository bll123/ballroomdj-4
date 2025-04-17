/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#if _sys_xattr
# include <sys/xattr.h>
#endif

#include "bdj4.h"
#include "bdj4arg.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "callback.h"
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
#include "osdirutil.h"
#include "osprocess.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathinfo.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"
#include "templateutil.h"
#include "ui.h"
#include "uiutils.h"
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
  INST_CREATE_LAUNCHER,
  INST_WIN_STARTUP,
  INST_INST_CLEAN_TMP,
  INST_SAVE_LOCALE,
  INST_VLC_CHECK,
  INST_VLC_DOWNLOAD,
  INST_VLC_INSTALL,
  INST_FINALIZE,
  INST_UPDATE_PROCESS_INIT,
  INST_UPDATE_PROCESS,
  INST_UPDATE_ALT_PROCESS_INIT,
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
  INST_W_BUTTON_TARGET_DIR,
  INST_W_BUTTON_BDJ3LOC_DIR,
  INST_W_BUTTON_EXIT,
  INST_W_BUTTON_INSTALL,
  INST_W_WINDOW,
  INST_W_STATUS_MSG,
  INST_W_RE_INSTALL,
  INST_W_FEEDBACK_MSG,
  INST_W_CONVERT,
  INST_W_CONV_FEEDBACK_MSG,
  INST_W_VLC_MSG,
  INST_W_STATUS_DISP,
  INST_W_TARGET,
  INST_W_BDJ3_LOC,
  INST_W_MAX,
};

enum {
  INST_TARGET,
  INST_BDJ3LOC,
};

typedef struct {
  installstate_t  instState;
  installstate_t  lastInstState;            // debugging
  callback_t      *callbacks [INST_CB_MAX];
  char            *home;
  char            *target;
  char            *hostname;
  char            *macospfx;
  char            rundir [MAXPATHLEN];      // installation dir with macospfx
  char            name [100];
  char            datatopdir [MAXPATHLEN];
  char            currdir [MAXPATHLEN];
  char            unpackdir [MAXPATHLEN];   // where the installer is unpacked
  char            vlcversion [80];
  char            dlfname [MAXPATHLEN];
  char            oldversion [MAXPATHLEN];
  char            bdj3version [MAXPATHLEN];
  loglevel_t      loglevel;
  webclient_t     *webclient;
  const char      *webresponse;
  size_t          webresplen;
  const char      *pleasewaitmsg;
  int             altidx;
  /* conversion */
  char            *bdj3loc;
  char            *tclshloc;
  slist_t         *convlist;
  slistidx_t      convidx;
  uiwcont_t       *wcont [INST_W_MAX];
  /* ati */
  char            ati [40];
  int             atiselect;
  /* flags */
  /* one of newinstall or updateinstall will be true */
  /* reinstall is simply the state of the checkbox */
  /* and for a reinstall, updateinstall must be true */
  bool            aborted;
  bool            bdjoptloaded;
  bool            clean;
  bool            convdirok;
  bool            convprocess;
  bool            convspecified;
  bool            guienabled;
  bool            invaltarget;
  bool            localespecified;
  bool            newinstall;
  bool            quiet;
  bool            readonly;
  bool            reinstall;          // button state
  bool            scrolltoend;
  bool            targetexists;
  bool            testregistration;
  bool            uiBuilt;
  bool            unattended;
  bool            updateinstall;
  bool            verbose;
  bool            vlcinstalled;
} installer_t;

static const char * const BDJ3_LOC_FILE = "install/bdj3loc.txt";

static void installerBuildUI (installer_t *installer);
static int  installerMainLoop (void *udata);
static bool installerExitCallback (void *udata);
static bool installerReinstallCBHandler (void *udata);
static bool installerConversionCBHandler (void *udata);
static bool installerTargetDirDialog (void *udata);
static void installerSetBDJ3LocEntry (installer_t *installer, const char *bdj3loc);
static bool installerBDJ3LocDirDialog (void *udata);
static int  installerValidateTarget (uiwcont_t *entry, const char *label, void *udata);
static int  installerValidateProcessTarget (installer_t *installer, const char *dir);
static void installerTargetFeedbackMsg (installer_t *installer);
static void installerSetConversionFlags (installer_t *installer);
static int  installerValidateBDJ3Loc (uiwcont_t *entry, const char *label, void *udata);
static int  installerValidateProcessBDJ3Loc (installer_t *installer, const char *dir);
static void installerSetPaths (installer_t *installer);
static void installerSetConvertStatus (installer_t *installer, int val);
static void installerConversionFeedbackMsg (installer_t *installer);
static bool installerInstallCallback (void *udata);

static void installerVerifyInstInit (installer_t *installer);
static void installerVerifyInstall (installer_t *installer);
static void installerPrepare (installer_t *installer);
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
static void installerCreateLauncher (installer_t *installer);
static void installerWinStartup (installer_t *installer);
static void installerInstCleanTmp (installer_t *installer);
static void installerSaveLocale (installer_t *installer);
static void installerVLCCheck (installer_t *installer);
static void installerVLCDownload (installer_t *installer);
static void installerVLCInstall (installer_t *installer);
static void installerFinalize (installer_t *installer);
static void installerUpdateProcessInit (installer_t *installer);
static void installerUpdateProcess (installer_t *installer);
static void installerUpdateAltProcessInit (installer_t *installer);
static void installerRegisterInit (installer_t *installer);
static void installerRegister (installer_t *installer);

static void installerCleanup (installer_t *installer);
static void installerDisplayText (installer_t *installer, const char *pfx, const char *txt, bool bold);
static void installerGetBDJ3Fname (installer_t *installer, char *buff, size_t len);
static void installerSetRundir (installer_t *installer, const char *dir);
static void installerVLCGetVersion (installer_t *installer);
static void installerCheckPackages (installer_t *installer);
static void installerWebResponseCallback (void *userdata, const char *resp, size_t len);
static void installerFailWorkingDir (installer_t *installer, const char *dir, const char *msg);
static void installerSetTargetDir (installer_t *installer, const char *fn);
static void installerSetBDJ3LocDir (installer_t *installer, const char *fn);
static void installerLoadBdjOpt (installer_t *installer);

int
main (int argc, char *argv[])
{
  installer_t   installer;
  char          tbuff [MAXPATHLEN];
  char          buff [MAXPATHLEN];
  FILE          *fh;
  int           c = 0;
  int           option_index = 0;
  bdj4arg_t     *bdj4arg;
  const char    *targ;

  static struct option bdj_options [] = {
    { "bdj3dir",    required_argument,  NULL,   '3' },
    { "bdj4installer",no_argument,      NULL,   0 },
    { "convert",    no_argument,        NULL,   'C' },
    { "locale",     required_argument,  NULL,   'L' },
    { "noclean",    no_argument,        NULL,   'c' },
    { "readonly",   no_argument,        NULL,   'R' },
    { "reinstall",  no_argument,        NULL,   'r' },
    { "targetdir",  required_argument,  NULL,   't' },
    { "testregistration", no_argument,  NULL,   'T' },
    { "unattended", no_argument,        NULL,   'U' },
    { "unpackdir",  required_argument,  NULL,   'u' },
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
    { "pli",        required_argument,  NULL,   0 },
    { "wait",       no_argument,        NULL,   0 },
    { "origcwd",      required_argument,  NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };


#if BDJ4_MEM_DEBUG
  mdebugInit ("inst");
#endif

  bdj4arg = bdj4argInit (argc, argv);

  buff [0] = '\0';

  installer.unpackdir [0] = '\0';
  installer.home = NULL;
  installer.instState = INST_INITIALIZE;
  installer.lastInstState = INST_INITIALIZE;
  installer.target = mdstrdup ("");
  installer.macospfx = "";
  installer.rundir [0] = '\0';
  installer.bdj3loc = mdstrdup ("");
  installer.convidx = 0;
  installer.convlist = NULL;
  installer.tclshloc = NULL;
  installer.currdir [0] = '\0';
  /* CONTEXT: installer: status message */
  installer.pleasewaitmsg = _("Please wait\xe2\x80\xa6");

  installer.aborted = false;
  installer.bdjoptloaded = false;
  installer.clean = true;
  installer.convdirok = false;
  installer.convprocess = false;
  installer.convspecified = false;
  installer.guienabled = true;
  installer.invaltarget = false;
  installer.localespecified = false;
  installer.newinstall = true;
  installer.quiet = false;
  installer.readonly = false;
  installer.reinstall = false;
  installer.scrolltoend = false;
  installer.targetexists = false;
  installer.testregistration = false;
  installer.uiBuilt = false;
  installer.unattended = false;
  installer.verbose = false;
  installer.vlcinstalled = false;

  for (int i = 0; i < INST_CB_MAX; ++i) {
    installer.callbacks [i] = NULL;
  }
  for (int i = 0; i < INST_W_MAX; ++i) {
    installer.wcont [i] = NULL;
  }

  installer.loglevel = LOG_IMPORTANT | LOG_BASIC | LOG_INFO | LOG_REDIR_INST;
  osGetCurrentDir (installer.currdir, sizeof (installer.currdir));
  installer.webclient = NULL;
  *installer.vlcversion = '\0';
  *installer.oldversion = '\0';
  *installer.bdj3version = '\0';

  /* the data in sysvars will not be correct.  don't use it.  */
  /* the installer only needs the home, hostname, os info and locale */
  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ, SYSVARS_FLAG_ALL);
  localeInit ();

  if (isMacOS ()) {
    installer.macospfx = MACOS_APP_PREFIX;
  }

  installer.hostname = sysvarsGetStr (SV_HOSTNAME);
  installer.home = sysvarsGetStr (SV_HOME);

  stpecpy (buff, buff + sizeof (buff), installer.home);
  if (isMacOS ()) {
    snprintf (buff, sizeof (buff), "%s/Applications", installer.home);
  }
  snprintf (installer.name, sizeof (installer.name), "%s%s",
      BDJ4_NAME, sysvarsGetStr (SV_BDJ4_DEVELOPMENT));
  instutilAppendNameToTarget (buff, sizeof (buff), installer.name, false);

  fh = fileopOpen (sysvarsGetStr (SV_FILE_INST_PATH), "r");
  if (fh != NULL) {
    (void) ! fgets (buff, sizeof (buff), fh);
    stringTrim (buff);
    mdextfclose (fh);
    fclose (fh);
    if (isWindows ()) {
      char    *tmp = NULL;

      tmp = regexReplaceLiteral (buff, "%USERNAME%", sysvarsGetStr (SV_USER));
      stpecpy (buff, buff + sizeof (buff), tmp);
      dataFree (tmp);
    }
  }

  /* at this point the target dir will have either a good default */
  /* or the saved target name */
  installerSetTargetDir (&installer, buff);

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "Cru:l:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg != NULL) {
          installer.loglevel = (loglevel_t) atol (optarg);
          installer.loglevel |= LOG_REDIR_INST;
        }
        break;
      }
      case 'T': {
        installer.testregistration = true;
        break;
      }
      case 'U': {
        installer.unattended = true;
        installer.guienabled = false;
#if _define_SIGCHLD
        osDefaultSignal (SIGCHLD);
#endif
        break;
      }
      case 'r': {
        installer.reinstall = true;
        break;
      }
      case 'R': {
        installer.readonly = true;
        break;
      }
      case 'C': {
        installer.convspecified = true;
        break;
      }
      case 'c': {
        installer.clean = false;
        break;
      }
      case 'u': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          stpecpy (installer.unpackdir, installer.unpackdir + sizeof (installer.unpackdir), targ);
        }
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
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          installerSetTargetDir (&installer, targ);
        }
        break;
      }
      case '3': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          installerSetBDJ3LocDir (&installer, targ);
        }
        break;
      }
      case 'L': {
        if (optarg != NULL) {
          sysvarsSetStr (SV_LOCALE, optarg);
          snprintf (tbuff, sizeof (tbuff), "%.2s", optarg);
          sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
          sysvarsSetNum (SVL_LOCALE_SET, 1);
          installer.localespecified = true;
          localeSetup ();
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  if (*installer.unpackdir == '\0') {
    if (! installer.quiet) {
      fprintf (stdout, "Error: unpackdir argument is required\n");
    }
    if (installer.verbose) {
      fprintf (stdout, "finish NG\n");
    }
    dataFree (installer.target);
    dataFree (installer.bdj3loc);
    bdj4argCleanup (bdj4arg);
#if BDJ4_MEM_DEBUG
    mdebugReport ();
    mdebugCleanup ();
#endif
    return 1;
  }

  /* only try to fetch the bdj3loc if the installer is not running */
  /* as unattended.  unattended installations will specify the bdj3loc */
  /* on the command line */
  if (! installer.unattended) {
    /* this only works if the installer.target is pointing at an existing */
    /* install of BDJ4 */
    installerGetBDJ3Fname (&installer, tbuff, sizeof (tbuff));
    if (*tbuff) {
      fh = fileopOpen (tbuff, "r");
      if (fh != NULL) {
        (void) ! fgets (buff, sizeof (buff), fh);
        stringTrim (buff);
        installerSetBDJ3LocDir (&installer, buff);
        mdextfclose (fh);
        fclose (fh);
      } else {
        char *fn;

        fn = locatebdj3 ();
        installerSetBDJ3LocDir (&installer, fn);
        mdfree (fn);
      }
    }
  }

  pathDisplayPath (installer.target, strlen (installer.target));

  if (installer.guienabled) {
    char      *uifont;
    uisetup_t uisetup;

    uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));

    uifont = sysvarsGetStr (SV_FONT_DEFAULT);
    if (uifont == NULL || ! *uifont) {
      uifont = "Arial Regular 11";
      if (isMacOS ()) {
        uifont = "Arial Regular 14";
      }
    }
    uisetup.uifont = uifont;
    uisetup.listingfont = uifont;
    uisetup.accentColor = INST_HL_COLOR;
    uisetup.errorColor = NULL;
    uisetup.markColor = NULL;
    uisetup.rowselColor = NULL;
    uisetup.rowhlColor = NULL;
    uisetup.mqbgColor = NULL;
    uiSetUICSS (&uisetup);
  }

  if (installer.guienabled) {
    installerBuildUI (&installer);
    osuiFinalize ();
  }

  installerCheckPackages (&installer);

  if (installer.guienabled) {
    uiEntrySetValue (installer.wcont [INST_W_TARGET], installer.target);
  }
  installerSetBDJ3LocEntry (&installer, installer.bdj3loc);
  installerConversionFeedbackMsg (&installer);

  while (installer.instState != INST_EXIT) {
    if (installer.instState != installer.lastInstState) {
      if (installer.verbose && ! installer.quiet) {
        fprintf (stderr, "state: %d\n", installer.instState);
      }
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "state: %d", installer.instState);
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
  bdj4argCleanup (bdj4arg);
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif

  return 0;
}

static void
installerBuildUI (installer_t *installer)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp;
  uiwcont_t     *szgrp;
  char          tbuff [100];
  char          imgbuff [MAXPATHLEN];

  szgrp = uiCreateSizeGroupHoriz ();

  uiLabelAddClass (INST_HL_CLASS, INST_HL_COLOR);
  uiSeparatorAddClass (INST_SEP_CLASS, INST_SEP_COLOR);

  stpecpy (imgbuff, imgbuff + sizeof (imgbuff), "img/bdj4_icon_inst.png");
  osuiSetIcon (imgbuff);

  stpecpy (imgbuff, imgbuff + sizeof (imgbuff), "img/bdj4_icon_inst.svg");
  /* CONTEXT: installer: window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Installer"), BDJ4_NAME);
  installer->callbacks [INST_CB_EXIT] = callbackInit (
      installerExitCallback, installer, NULL);
  installer->wcont [INST_W_WINDOW] = uiCreateMainWindow (
      installer->callbacks [INST_CB_EXIT],
      tbuff, imgbuff);
  uiWindowSetDefaultSize (installer->wcont [INST_W_WINDOW], 1000, 600);

  vbox = uiCreateVertBox ();
  uiWindowPackInWindow (installer->wcont [INST_W_WINDOW], vbox);
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);

  /* begin line : status message */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  installer->wcont [INST_W_STATUS_MSG] = uiCreateLabel ("");
  uiWidgetAddClass (installer->wcont [INST_W_STATUS_MSG], INST_HL_CLASS);
  uiBoxPackEndExpand (hbox, installer->wcont [INST_W_STATUS_MSG]);
  uiWidgetAlignHorizEnd (installer->wcont [INST_W_STATUS_MSG]);

  /* begin line : target instructions */
  uiwidgetp = uiCreateLabel (
      /* CONTEXT: installer: where BDJ4 gets installed */
      _("Enter the destination folder where BDJ4 will be installed."));
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* begin line : target entry */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiEntryInit (60, MAXPATHLEN);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  installer->wcont [INST_W_TARGET] = uiwidgetp;

  installer->callbacks [INST_CB_TARGET_DIR] = callbackInit (
      installerTargetDirDialog, installer, NULL);
  uiwidgetp = uiCreateButton ("inst-tgt-folder",
      installer->callbacks [INST_CB_TARGET_DIR],
      "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);
  installer->wcont [INST_W_BUTTON_TARGET_DIR] = uiwidgetp;

  uiwcontFree (hbox);

  /* begin line : re-install bdj4 */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: installer: checkbox: re-install BDJ4 */
  installer->wcont [INST_W_RE_INSTALL] = uiCreateCheckButton (_("Re-Install"),
      installer->reinstall);
  uiBoxPackStart (hbox, installer->wcont [INST_W_RE_INSTALL]);
  installer->callbacks [INST_CB_REINST] = callbackInit (
      installerReinstallCBHandler, installer, NULL);
  uiToggleButtonSetCallback (installer->wcont [INST_W_RE_INSTALL],
      installer->callbacks [INST_CB_REINST]);

  installer->wcont [INST_W_FEEDBACK_MSG] = uiCreateLabel ("");
  uiWidgetAddClass (installer->wcont [INST_W_FEEDBACK_MSG], INST_HL_CLASS);
  uiBoxPackStart (hbox, installer->wcont [INST_W_FEEDBACK_MSG]);

  uiwcontFree (hbox);

  /* begin line : separator */
  uiwidgetp = uiCreateHorizSeparator ();
  uiWidgetAddClass (uiwidgetp, INST_SEP_CLASS);
  uiBoxPackStart (vbox, uiwidgetp);
  uiWidgetSetMarginTop (uiwidgetp, 4);
  uiWidgetSetMarginBottom (uiwidgetp, 2);
  uiwcontFree (uiwidgetp);

  /* begin line : instructions a */
  /* conversion process */
  snprintf (tbuff, sizeof (tbuff),
      /* CONTEXT: installer: asking where BallroomDJ 3 is installed */
      _("Enter the folder where %s is installed."), BDJ3_NAME);
  uiwidgetp = uiCreateLabel (tbuff);
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* begin line : instructions b */
  uiwidgetp = uiCreateLabel (
      /* CONTEXT: installer: instructions */
      _("If there is no BallroomDJ 3 installation, leave the entry blank."));
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* begin line : instructions c */
  uiwidgetp = uiCreateLabel (
      /* CONTEXT: installer: instructions */
      _("The conversion process will only run for new installations and for re-installations."));
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* begin line : bdj3 location */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: installer: label for entry field asking for BDJ3 location */
  snprintf (tbuff, sizeof (tbuff), _("%s Location"), BDJ3_NAME);
  uiwidgetp = uiCreateColonLabel (tbuff);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (60, MAXPATHLEN);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  installer->wcont [INST_W_BDJ3_LOC] = uiwidgetp;

  installer->callbacks [INST_CB_BDJ3LOC_DIR] = callbackInit (
      installerBDJ3LocDirDialog, installer, NULL);
  uiwidgetp = uiCreateButton ("inst-bdj3loc-folder",
      installer->callbacks [INST_CB_BDJ3LOC_DIR],
      "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 0);
  installer->wcont [INST_W_BUTTON_BDJ3LOC_DIR] = uiwidgetp;

  uiwcontFree (hbox);

  /* begin line : conversion checkbox */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: installer: checkbox: convert the BallroomDJ 3 installation */
  snprintf (tbuff, sizeof (tbuff), _("Convert %s"), BDJ3_NAME);
  installer->wcont [INST_W_CONVERT] = uiCreateCheckButton (tbuff, 0);
  uiBoxPackStart (hbox, installer->wcont [INST_W_CONVERT]);
  installer->callbacks [INST_CB_CONV] = callbackInit (
      installerConversionCBHandler, installer, NULL);
  uiToggleButtonSetCallback (installer->wcont [INST_W_CONVERT],
      installer->callbacks [INST_CB_CONV]);

  installer->wcont [INST_W_CONV_FEEDBACK_MSG] = uiCreateLabel ("");
  uiWidgetAddClass (installer->wcont [INST_W_CONV_FEEDBACK_MSG], INST_HL_CLASS);
  uiBoxPackStart (hbox, installer->wcont [INST_W_CONV_FEEDBACK_MSG]);

  uiwcontFree (hbox);

  /* begin line : separator */
  uiwidgetp = uiCreateHorizSeparator ();
  uiWidgetAddClass (uiwidgetp, INST_SEP_CLASS);
  uiBoxPackStart (vbox, uiwidgetp);
  uiWidgetSetMarginTop (uiwidgetp, 4);
  uiWidgetSetMarginBottom (uiwidgetp, 2);
  uiwcontFree (uiwidgetp);

  /* begin line : vlc message */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel ("VLC");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  installer->wcont [INST_W_VLC_MSG] = uiCreateLabel ("");
  uiWidgetAddClass (installer->wcont [INST_W_VLC_MSG], INST_HL_CLASS);
  uiBoxPackStart (hbox, installer->wcont [INST_W_VLC_MSG]);
  uiWidgetSetMarginStart (installer->wcont [INST_W_VLC_MSG], 4);

  uiwcontFree (hbox);

  /* begin line : buttons */
  /* button box */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateButton ("inst-exit",
      installer->callbacks [INST_CB_EXIT],
      /* CONTEXT: installer: exits the installer */
      _("Exit"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  installer->wcont [INST_W_BUTTON_EXIT] = uiwidgetp;

  installer->callbacks [INST_CB_INSTALL] = callbackInit (
      installerInstallCallback, installer, NULL);
  uiwidgetp = uiCreateButton ("inst-install",
      installer->callbacks [INST_CB_INSTALL],
      /* CONTEXT: installer: start the installation process */
      _("Install"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  installer->wcont [INST_W_BUTTON_INSTALL] = uiwidgetp;

  uiwidgetp = uiTextBoxCreate (250, INST_HL_COLOR);
  uiTextBoxSetReadonly (uiwidgetp);
  uiTextBoxHorizExpand (uiwidgetp);
  uiTextBoxVertExpand (uiwidgetp);
  uiBoxPackStartExpand (vbox, uiwidgetp);
  installer->wcont [INST_W_STATUS_DISP] = uiwidgetp;

  uiWidgetShowAll (installer->wcont [INST_W_WINDOW]);
  installer->uiBuilt = true;

  uiwcontFree (hbox);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  uiEntrySetValidate (installer->wcont [INST_W_TARGET], "",
      installerValidateTarget, installer, UIENTRY_DELAYED);
  uiEntrySetValidate (installer->wcont [INST_W_BDJ3_LOC], "",
      installerValidateBDJ3Loc, installer, UIENTRY_DELAYED);
}

static int
installerMainLoop (void *udata)
{
  installer_t *installer = udata;

  if (installer->guienabled) {
    uiUIProcessEvents ();
  }

  if (installer->guienabled) {
    uiEntryValidate (installer->wcont [INST_W_TARGET], false);
    uiEntryValidate (installer->wcont [INST_W_BDJ3_LOC], false);
  }

  if (installer->guienabled && installer->scrolltoend) {
    uiTextBoxScrollToEnd (installer->wcont [INST_W_STATUS_DISP]);
    installer->scrolltoend = false;
    uiUIProcessWaitEvents ();
    /* go through the main loop once more */
    return true;
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
      installerPrepare (installer);
      break;
    }
    case INST_WAIT_USER: {
      /* do nothing */
      if (installer->unattended) {
        installer->instState = INST_FINISH;
      }
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
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "BDJ4 version: %s", sysvarsGetStr (SV_BDJ4_VERSION));
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "target: %s", installer->target);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "initial bdj3loc: %s", installer->bdj3loc);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "new-install: %d", installer->newinstall);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "update: %d", installer->updateinstall);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "re-install: %d", installer->reinstall);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "convert: %d", installer->convprocess);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "vlc-inst: %d", installer->vlcinstalled);

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
    case INST_CREATE_LAUNCHER: {
      installerCreateLauncher (installer);
      break;
    }
    case INST_WIN_STARTUP: {
      installerWinStartup (installer);
      break;
    }
    case INST_INST_CLEAN_TMP: {
      installerInstCleanTmp (installer);
      break;
    }
    case INST_SAVE_LOCALE: {
      installerSaveLocale (installer);
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
    case INST_FINALIZE: {
      installerFinalize (installer);
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
    case INST_UPDATE_ALT_PROCESS_INIT: {
      installerUpdateAltProcessInit (installer);
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
      if (! installer->aborted) {
        /* CONTEXT: installer: status message */
        installerDisplayText (installer, INST_DISP_FIN, _("Installation complete."), true);
      }
      installer->aborted = false;

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

  return true;
}

static bool
installerReinstallCBHandler (void *udata)
{
  installer_t   *installer = udata;
  int           nval;

  nval = uiToggleButtonIsActive (installer->wcont [INST_W_RE_INSTALL]);
  installer->reinstall = nval;
  installerTargetFeedbackMsg (installer);
  return UICB_CONT;
}

static bool
installerConversionCBHandler (void *udata)
{
  installer_t   *installer = udata;
  int           nval;

  /* need to force conversion off if the dir is not valid or empty */
  if (! installer->convdirok) {
    installerSetConvertStatus (installer, UI_TOGGLE_BUTTON_OFF);
    installer->convprocess = false;
  }

  nval = uiToggleButtonIsActive (installer->wcont [INST_W_CONVERT]);
  installer->convprocess = false;
  if (nval && *installer->bdj3loc) {
    installer->convprocess = true;
  }

  installerConversionFeedbackMsg (installer);

  return UICB_CONT;
}

static int
installerValidateTarget (uiwcont_t *entry, const char *label, void *udata)
{
  installer_t   *installer = udata;
  const char    *dir;
  char          tbuff [MAXPATHLEN];
  int           rc = UIENTRY_ERROR;

  if (! installer->guienabled) {
    return UIENTRY_ERROR;
  }

  if (! installer->uiBuilt) {
    return UIENTRY_RESET;
  }

  if (installer->invaltarget) {
    return UIENTRY_RESET;
  }

  installer->invaltarget = true;

  dir = uiEntryGetValue (installer->wcont [INST_W_TARGET]);
  stpecpy (tbuff, tbuff + sizeof (tbuff), dir);
  pathNormalizePath (tbuff, strlen (tbuff));

  /* only call the validation process if the directory has changed */
  if (strcmp (tbuff, installer->target) != 0) {
    rc = installerValidateProcessTarget (installer, tbuff);
  } else {
    rc = UIENTRY_OK;
  }

  installer->invaltarget = false;
  return rc;
}

static int
installerValidateProcessTarget (installer_t *installer, const char *dir)
{
  int         rc = UIENTRY_ERROR;
  bool        exists = false;
  bool        found = false;
  char        tbuff [MAXPATHLEN];
  char        tdir [MAXPATHLEN];
  char        *p = tdir;
  char        *end = tdir + sizeof (tdir);

  p = stpecpy (p, end, dir);

  if (isMacOS ()) {
    /* on macos, the .app extension must always exist */
    if (strstr (tdir, MACOS_APP_EXT) == NULL) {
      p = stpecpy (p, end, MACOS_APP_EXT);
    }
  }

  if (fileopIsDirectory (tdir)) {
    exists = true;
    found = instutilCheckForExistingInstall (tdir, installer->macospfx);
    if (! found) {
      stpecpy (tbuff, tbuff + sizeof (tbuff), tdir);
      instutilAppendNameToTarget (tbuff, sizeof (tbuff), NULL, true);
      exists = fileopIsDirectory (tbuff);
      if (exists) {
        found = instutilCheckForExistingInstall (tbuff, installer->macospfx);
        if (found) {
          stpecpy (tdir, tdir + sizeof (tdir), tbuff);
        }
      }
    }

    /* do not try to overwrite an existing alternate installation */
    if (exists && found &&
        instutilIsStandardInstall (tdir, installer->macospfx)) {
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
    pi = pathInfo (tdir);
    if (pi->dlen > 0) {
      pathInfoGetDir (pi, tmp, sizeof (tmp));
      if (! fileopIsDirectory (tmp)) {
        rc = UIENTRY_ERROR;
      }
    }
    pathInfoFree (pi);
  }

  installer->newinstall = false;
  installer->updateinstall = false;
  installer->targetexists = false;
  if (exists) {
    installer->targetexists = true;
  }

  if (rc == UIENTRY_OK) {
    /* set the target directory information */
    installerSetTargetDir (installer, tdir);
    if (exists) {
      if (found) {
        installer->newinstall = false;
        installer->updateinstall = true;
      }
    }
    if (! exists) {
      installer->newinstall = true;
    }
    installerSetConversionFlags (installer);
  }

  installerTargetFeedbackMsg (installer);

  return rc;
}

static void
installerTargetFeedbackMsg (installer_t *installer)
{
  char          tbuff [MAXPATHLEN];

  /* one of newinstall or updateinstall must be set */
  /* reinstall matches the re-install checkbox */
  /* if targetexists is true, and updateinstall is false, the target */
  /* folder is an existing folder, but not a bdj4 installation */

  if (installer->updateinstall && installer->reinstall) {
    /* CONTEXT: installer: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("Re-install %s.  Overwrites existing configuration."), BDJ4_NAME);
    uiLabelSetText (installer->wcont [INST_W_FEEDBACK_MSG], tbuff);
    /* CONTEXT: installer: start the installation process */
    uiButtonSetText (installer->wcont [INST_W_BUTTON_INSTALL], _("Install"));
  }
  if (installer->updateinstall && ! installer->reinstall) {
    /* CONTEXT: installer: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("Updating existing %s installation."), BDJ4_NAME);
    uiLabelSetText (installer->wcont [INST_W_FEEDBACK_MSG], tbuff);
    /* CONTEXT: installer: start the upgrade process */
    uiButtonSetText (installer->wcont [INST_W_BUTTON_INSTALL], _("Upgrade"));
  }
  if (installer->newinstall) {
    /* CONTEXT: installer: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("New %s installation."), BDJ4_NAME);
    uiLabelSetText (installer->wcont [INST_W_FEEDBACK_MSG], tbuff);
    /* CONTEXT: installer: start the installation process */
    uiButtonSetText (installer->wcont [INST_W_BUTTON_INSTALL], _("Install"));
  }
  if (installer->targetexists && ! installer->updateinstall) {
    /* CONTEXT: installer: the selected folder exists and is not a BDJ4 installation */
    uiLabelSetText (installer->wcont [INST_W_FEEDBACK_MSG], _("Error: Folder already exists."));
  }
  if (! installer->targetexists && ! installer->newinstall && ! installer->updateinstall) {
    /* CONTEXT: installer: the selected folder is not valid */
    uiLabelSetText (installer->wcont [INST_W_FEEDBACK_MSG], _("Invalid Folder"));
  }

  return;
}

static void
installerSetConversionFlags (installer_t *installer)
{
  /* set the conversion flags */
  if (installer->unattended) {
    if (installer->convspecified) {
      installer->convprocess = true;
    }
    return;
  }

  if (installer->convdirok) {
    if (installer->newinstall && *installer->bdj3loc) {
      installerSetConvertStatus (installer, UI_TOGGLE_BUTTON_ON);
      installer->convprocess = true;
    }
    if (installer->updateinstall) {
      /* do not automatically convert on a re-install */
      /* never convert on an update */
      installerSetConvertStatus (installer, UI_TOGGLE_BUTTON_OFF);
      installer->convprocess = false;
    }
  } else {
    installerSetConvertStatus (installer, UI_TOGGLE_BUTTON_OFF);
    installer->convprocess = false;
  }

  installerConversionFeedbackMsg (installer);
}

static int
installerValidateBDJ3Loc (uiwcont_t *entry, const char *label, void *udata)
{
  installer_t   *installer = udata;
  const char    *dir;
  char          tbuff [MAXPATHLEN];
  int           rc = UIENTRY_ERROR;

  if (! installer->guienabled) {
    return UIENTRY_ERROR;
  }

  if (! installer->uiBuilt) {
    return UIENTRY_RESET;
  }

  /* bdj3 location validation */

  dir = uiEntryGetValue (installer->wcont [INST_W_BDJ3_LOC]);
  stpecpy (tbuff, tbuff + sizeof (tbuff), dir);
  pathNormalizePath (tbuff, strlen (tbuff));

  installer->convdirok = false;
  rc = installerValidateProcessBDJ3Loc (installer, tbuff);

  return rc;
}

static int
installerValidateProcessBDJ3Loc (installer_t *installer, const char *dir)
{
  int       rc = UIENTRY_ERROR;

  if (*dir == '\0') {
    rc = UIENTRY_OK;
  } else {
    if (! isMacOS ()) {
      if (locationcheck (dir)) {
        installer->convdirok = true;
        rc = UIENTRY_OK;
      }
    } else {
      char  *fn;

      fn = locatebdj3 ();
      if (fn != NULL) {
        installer->convdirok = true;
        rc = UIENTRY_OK;
        mdfree (fn);
      }
    }
  }

  if (rc == UIENTRY_ERROR) {
    installerSetConvertStatus (installer, UI_TOGGLE_BUTTON_OFF);
  }

  if (rc == UIENTRY_OK) {
    installerSetBDJ3LocDir (installer, dir);
  }

  installerSetConversionFlags (installer);

  return rc;
}

static void
installerSetPaths (installer_t *installer)
{
  installerSetRundir (installer, installer->target);

  stpecpy (installer->datatopdir, installer->datatopdir + sizeof (installer->datatopdir), installer->rundir);
  if (isMacOS ()) {
    snprintf (installer->datatopdir, sizeof (installer->datatopdir),
        "%s%s/%s", installer->home, MACOS_DIR_LIBDATA, installer->name);
  }
  if (installer->targetexists) {
    installerLoadBdjOpt (installer);
  }
}

static bool
installerTargetDirDialog (void *udata)
{
  installer_t *installer = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;

  selectdata = uiSelectInit (installer->wcont [INST_W_WINDOW],
      /* CONTEXT: installer: dialog title for selecting install location */
      _("Install Location"),
      uiEntryGetValue (installer->wcont [INST_W_TARGET]),
      NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    char        tbuff [MAXPATHLEN];

    stpecpy (tbuff, tbuff + sizeof (tbuff), fn);
    /* after the user selected a folder via a button, */
    /* want the /BDJ4 appended */
    instutilAppendNameToTarget (tbuff, sizeof (tbuff), NULL, false);
    /* the validation routine gets called upon set, */
    /* which will call the set-target routine */
    pathDisplayPath (tbuff, sizeof (tbuff));
    uiEntrySetValue (installer->wcont [INST_W_TARGET], tbuff);
    mdfree (fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected target loc: %s", installer->target);
  }
  uiSelectFree (selectdata);
  return UICB_CONT;
}

static void
installerSetBDJ3LocEntry (installer_t *installer, const char *bdj3loc)
{
  char    tbuff [MAXPATHLEN];

  stpecpy (tbuff, tbuff + sizeof (tbuff), bdj3loc);
  pathDisplayPath (tbuff, sizeof (tbuff));
  if (installer->guienabled) {
    uiEntrySetValue (installer->wcont [INST_W_BDJ3_LOC], tbuff);
  }
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
  selectdata = uiSelectInit (installer->wcont [INST_W_WINDOW],
      tbuff, uiEntryGetValue (installer->wcont [INST_W_BDJ3_LOC]), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    installerSetBDJ3LocEntry (installer, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected bdj3 loc: %s", installer->bdj3loc);
    mdfree (fn);
  }
  uiSelectFree (selectdata);
  return UICB_CONT;
}

static void
installerSetConvertStatus (installer_t *installer, int state)
{
  if (! installer->guienabled) {
    return;
  }

  uiToggleButtonSetValue (installer->wcont [INST_W_CONVERT], state);
}

static void
installerConversionFeedbackMsg (installer_t *installer)
{
  int           nval = false;
  char          *tptr;
  char          tbuff [200];

  if (! installer->guienabled) {
    return;
  }

  nval = uiToggleButtonIsActive (installer->wcont [INST_W_CONVERT]);

  if (nval) {
    /* CONTEXT: installer: message indicating the conversion action that will be taken */
    tptr = _("%s data will be converted.");
    snprintf (tbuff, sizeof (tbuff), tptr, BDJ3_NAME);
    uiLabelSetText (installer->wcont [INST_W_CONV_FEEDBACK_MSG], tbuff);
  } else {
    if (installer->convdirok) {
      /* CONTEXT: installer: message indicating the conversion action that will be taken */
      tptr = _("The conversion process will not be run.");
      uiLabelSetText (installer->wcont [INST_W_CONV_FEEDBACK_MSG], tptr);
    } else if (! *installer->bdj3loc) {
      /* CONTEXT: installer: message indicating the conversion action that will be taken */
      tptr = _("No folder specified.");
      uiLabelSetText (installer->wcont [INST_W_CONV_FEEDBACK_MSG], tptr);
    } else {
      /* CONTEXT: installer: the location entered is not a valid BDJ3 location. */
      snprintf (tbuff, sizeof (tbuff), _("Not a valid %s folder."), BDJ3_NAME);
      uiLabelSetText (installer->wcont [INST_W_CONV_FEEDBACK_MSG], tbuff);
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

/* installation routines */

static void
installerVerifyInstInit (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Verifying installation."), false);
  installerDisplayText (installer, INST_DISP_STATUS, installer->pleasewaitmsg, false);
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], installer->pleasewaitmsg);

  /* the unpackdir is not necessarily the same as the rundir */
  /* on mac os, they are different */
  if (osChangeDir (installer->unpackdir) < 0) {
    installerFailWorkingDir (installer, installer->unpackdir, "verifyinstinit");
    return;
  }

  installer->instState = INST_VERIFY_INSTALL;
}

static void
installerVerifyInstall (installer_t *installer)
{
  char        tmp [40];
  char        tbuff [MAXPATHLEN];
  const char  *targv [3];

  if (isWindows ()) {
    /* verification on windows is too slow */
    stpecpy (tmp, tmp + sizeof (tmp), "OK");
  } else {
    snprintf (tbuff, sizeof (tbuff), ".%s/install/verifychksum.sh",
        installer->macospfx);
    targv [0] = tbuff;
    targv [1] = NULL;
    osProcessPipe (targv, OS_PROC_WAIT, tmp, sizeof (tmp), NULL);
  }

  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], "");
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
installerPrepare (installer_t *installer)
{
  /* to get initial feedback messages displayed and to set variables */
  installerValidateProcessTarget (installer, installer->target);
  installerValidateProcessBDJ3Loc (installer, installer->bdj3loc);
  installerSetPaths (installer);

  if (! installer->guienabled && ! installer->unattended) {
    fprintf (stderr, "Error: no installation specified\n");
    if (installer->verbose) {
      fprintf (stdout, "finish NG\n");
    }
    installer->instState = INST_FINISH;
  } else if (installer->unattended) {
    installer->instState = INST_INIT;
  } else {
    /* gui is enabled, wait for install button */
    installer->instState = INST_WAIT_USER;
  }
}

static void
installerInstInit (installer_t *installer)
{
  /* no possible installation could be made */
  /* this is generally an attempt to install to a folder that exists */
  /* but no bdj4 installation was found */
  if (! installer->newinstall && ! installer->updateinstall) {
    /* CONTEXT: installer: status message */
    installerDisplayText (installer, INST_DISP_ERROR, _("Error: Folder already exists."), false);
    /* CONTEXT: installer: status message */
    installerDisplayText (installer, INST_DISP_ERROR, _("Installation aborted."), false);
    installer->aborted = true;
    installer->instState = INST_WAIT_USER;
    return;
  }

  /* a bad conversion dir will turn off the convprocess flag */

  installerSetPaths (installer);
  installer->instState = INST_SAVE_TARGET;
}

static void
installerSaveTargetDir (installer_t *installer)
{
  FILE        *fh;

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Saving install location."), false);

  diropMakeDir (sysvarsGetStr (SV_DIR_CONFIG));
  fh = fileopOpen (sysvarsGetStr (SV_FILE_INST_PATH), "w");
  if (fh != NULL) {
    char  *tmp = installer->target;

    if (isWindows ()) {
      /* On windows, the uninstall batch script reads the install path from */
      /* a text file.  International characters cannot be processed */
      /* in this manner.  Therefore replace the username with the */
      /* environment variable, and the installer and any windows scripts */
      /* must replace the variable with the user name. */

      tmp = regexReplaceLiteral (installer->target, sysvarsGetStr (SV_USER),
          WINUSERNAME);
    }
    fprintf (fh, "%s\n", tmp);
    mdextfclose (fh);
    fclose (fh);
    if (isWindows ()) {
      dataFree (tmp);
    }
  }

  installer->instState = INST_MAKE_TARGET;
}

static void
installerMakeTarget (installer_t *installer)
{
  char            tbuff [MAXPATHLEN];
  sysversinfo_t   *versinfo;

  diropMakeDir (installer->target);
  diropMakeDir (installer->rundir);

  *installer->oldversion = '\0';
  snprintf (tbuff, sizeof (tbuff), "%s/VERSION.txt", installer->target);
  versinfo = sysvarsParseVersionFile (tbuff);
  instutilOldVersionString (versinfo, installer->oldversion, sizeof (installer->oldversion));
  sysvarsParseVersionFileFree (versinfo);

  installer->instState = INST_COPY_START;
}

static void
installerCopyStart (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Copying files."), false);
  installerDisplayText (installer, INST_DISP_STATUS, installer->pleasewaitmsg, false);
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], installer->pleasewaitmsg);

  /* the unpackdir is not necessarily the same as the rundir */
  /* on mac os, they are different */
  if (osChangeDir (installer->unpackdir) < 0) {
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

  /* due to various reasons, symlinks were not being preserved on macos */
  /* during the installation process. */
  /* in order to properly install the locale/en and locale/nl symlinks, */
  /* these two directories must be removed first */
  if (isMacOS ()) {
    snprintf (tbuff, sizeof (tbuff), "%s%s/locale/en",
        installer->target, installer->macospfx);
    if (! osIsLink (tbuff) && fileopIsDirectory (tbuff)) {
      diropDeleteDir (tbuff, DIROP_ALL);
    }
    snprintf (tbuff, sizeof (tbuff), "%s%s/locale/nl",
        installer->target, installer->macospfx);
    if (! osIsLink (tbuff) && fileopIsDirectory (tbuff)) {
      diropDeleteDir (tbuff, DIROP_ALL);
    }
  }

  if (isWindows ()) {
    char      *trundir = NULL;

    *tmp = '\0';
    trundir = regexReplaceLiteral (installer->rundir,
        sysvarsGetStr (SV_USER), WINUSERNAME);
    stpecpy (tmp, tmp + sizeof (tmp), trundir);
    dataFree (trundir);
    pathDisplayPath (tmp, sizeof (tmp));

    snprintf (tbuff, sizeof (tbuff),
        "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl . \"%s\"",
        tmp);
  } else {
    snprintf (tbuff, sizeof (tbuff), "rsync -aS . '%s'", installer->target);
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "copy files: %s", tbuff);
  (void) ! system (tbuff);
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], "");
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_STATUS, _("Copy finished."), false);

  if (installer->readonly) {
    installer->instState = INST_VLC_CHECK;
  } else {
    installer->instState = INST_MAKE_DATA_TOP;
  }
}

static void
installerMakeDataTop (installer_t *installer)
{
  diropMakeDir (installer->datatopdir);

  if (osChangeDir (installer->datatopdir) < 0) {
    installerFailWorkingDir (installer, installer->datatopdir, "makedatatop");
    return;
  }

  installer->instState = INST_CREATE_DIRS;
}

static void
installerCreateDirs (installer_t *installer)
{
  if (installer->updateinstall) {
    installerLoadBdjOpt (installer);
  }

  if (installer->updateinstall && ! installer->reinstall) {
    /* if it is an update only, do not create dirs or copy templates */
    installer->instState = INST_CONVERT_START;
    return;
  }

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Creating folder structure."), false);

  /* this will create the directories necessary for the configs */
  /* namely: profile00, <hostname>, <hostname>/profile00 */
  instutilCreateDataDirectories ();
  /* create the directories that are not included in the distribution */
  diropMakeDir ("tmp");
  diropMakeDir ("http");
  diropMakeDir ("img/profile00");

  installer->instState = INST_COPY_TEMPLATES_INIT;
}

static void
installerCopyTemplatesInit (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Copying template files."), false);
  installer->instState = INST_COPY_TEMPLATES;
}

static void
installerCopyTemplates (installer_t *installer)
{
  char    from [MAXPATHLEN];
  char    to [MAXPATHLEN];

  if (osChangeDir (installer->datatopdir) < 0) {
    installerFailWorkingDir (installer, installer->datatopdir, "copytemplates");
    return;
  }

  instutilCopyTemplates ();
  instutilCopyHttpFiles ();     /* copies led_on, led_off */
  templateImageCopy (NULL);

  if (isMacOS ()) {
    snprintf (to, sizeof (to), "%s/.themes", installer->home);
    diropMakeDir (to);

    snprintf (from, sizeof (from), "../Applications/BDJ4.app/Contents/MacOS/plocal/share/themes/Mojave-dark-solid");
    snprintf (to, sizeof (to), "%s/.themes/Mojave-dark-solid", installer->home);
    /* be nice, and don't remove existing links/themes */
    if (! fileopIsDirectory (to)) {
      filemanipLinkCopy (from, to);
    }

    snprintf (from, sizeof (from), "../Applications/BDJ4.app/Contents/MacOS/plocal/share/themes/Mojave-light-solid");
    snprintf (to, sizeof (to), "%s/.themes/Mojave-light-solid", installer->home);
    if (! fileopIsDirectory (to)) {
      filemanipLinkCopy (from, to);
    }
  }

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
  char    *b3vp;
  char    *b3vend;


  if (! installer->convprocess) {
    installer->instState = INST_CREATE_LAUNCHER;
    return;
  }

  if (*installer->bdj3loc == '\0') {
    installer->instState = INST_CREATE_LAUNCHER;
    return;
  }

  if (osChangeDir (installer->rundir) < 0) {
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
      mdextfclose (fh);
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
    installer->instState = INST_CREATE_LAUNCHER;
    return;
  }

  *installer->bdj3version = '\0';
  snprintf (tbuff, sizeof (tbuff), "%s/VERSION.txt",
      installer->bdj3loc);
  b3vp = installer->bdj3version;
  b3vend = installer->bdj3version + sizeof (installer->bdj3version);
  if (isMacOS () && ! fileopFileExists (tbuff)) {
    snprintf (tbuff, sizeof (tbuff),
        "%s/Applications/BallroomDJ.app%s/VERSION.txt",
        installer->home, MACOS_APP_PREFIX);
  }
  if (fileopFileExists (tbuff)) {
    data = filedataReadAll (tbuff, NULL);
    tp = strtok_r (data, "\r\n", &tokptr);
    while (tp != NULL) {
      vnm = strtok_r (tp, "=", &tokptrb);
      p = strtok_r (NULL, "=", &tokptrb);
      if (strcmp (vnm, "VERSION") == 0) {
        b3vp = stpecpy (b3vp, b3vend, p);
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
  snprintf (tbuff, sizeof (tbuff), "%s/%s/%" PRId64 "/tcl/bin/tclsh",
      installer->bdj3loc, sysvarsGetStr (SV_OS_NAME), sysvarsGetNum (SVL_OS_BITS));
  locs [locidx++] = mdstrdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/Applications/BallroomDJ.app/Contents/%s/%" PRId64 "/tcl/bin/tclsh",
      installer->home, sysvarsGetStr (SV_OS_NAME), sysvarsGetNum (SVL_OS_BITS));
  locs [locidx++] = mdstrdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/local/bin/tclsh", installer->home);
  locs [locidx++] = mdstrdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/bin/tclsh", installer->home);
  locs [locidx++] = mdstrdup (tbuff);
  /* for testing; low priority */
  snprintf (tbuff, sizeof (tbuff), "%s/../%s/%" PRId64 "/tcl/bin/tclsh",
      installer->bdj3loc, sysvarsGetStr (SV_OS_NAME), sysvarsGetNum (SVL_OS_BITS));
  locs [locidx++] = mdstrdup (tbuff);
  locs [locidx++] = mdstrdup ("/opt/local/bin/tclsh");
  locs [locidx++] = mdstrdup ("/usr/bin/tclsh");
  locs [locidx++] = NULL;

  locidx = 0;
  while (locs [locidx] != NULL) {
    stpecpy (tbuff, tbuff + sizeof (tbuff), locs [locidx]);
    if (isWindows ()) {
      snprintf (tbuff, sizeof (tbuff), "%s.exe", locs [locidx]);
    }

    if (installer->tclshloc == NULL && fileopFileExists (tbuff)) {
      pathDisplayPath (tbuff, sizeof (tbuff));
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
    installer->instState = INST_CREATE_LAUNCHER;
    return;
  }

  installer->instState = INST_CONVERT;
}

static void
installerConvert (installer_t *installer)
{
  const char  *fn;
  char        buffa [MAXPATHLEN];
  char        buffb [MAXPATHLEN];
  const char  *targv [15];

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
  installerLoadBdjOpt (installer);

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_STATUS, _("Conversion complete."), false);
  installer->instState = INST_CREATE_LAUNCHER;
}

static void
installerCreateLauncher (installer_t *installer)
{
  if (osChangeDir (installer->rundir) < 0) {
    installerFailWorkingDir (installer, installer->rundir, "CreateLauncher");
    return;
  }

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Creating shortcut."), false);

  /* handles linux and windows desktop shortcut */
  /* for macos, the app is already there, */
  /* so avoid running the creation script */
  if (! isMacOS ()) {
    instutilCreateLauncher (installer->name, installer->rundir, installer->rundir, 0);
  }

  if (isMacOS ()) {
#if _lib_symlink
    char        buff [MAXPATHLEN];

    /* on macos, the startup program must be a gui program, otherwise */
    /* the dock icon is not correct */
    /* this must exist and match the name of the app */
    (void) ! symlink ("bin/bdj4g", "BDJ4");

    /* remove any old name.app on macos */
    snprintf (buff, sizeof (buff), "%s/Desktop/%s%s",
        installer->home, installer->name, MACOS_APP_EXT);
    if (osIsLink (buff)) {
      fileopDelete (buff);
    }

    /* desktop shortcut pointing to .app */
    snprintf (buff, sizeof (buff), "%s/Desktop/%s",
        installer->home, installer->name);
    (void) ! symlink (installer->target, buff);
#endif
  }

  installer->instState = INST_WIN_STARTUP;
}

static void
installerWinStartup (installer_t *installer)
{
  if (osChangeDir (installer->rundir) < 0) {
    installerFailWorkingDir (installer, installer->rundir, "winstartup");
    return;
  }

  if (isWindows ()) {
    char  tbuff [MAXPATHLEN];

    snprintf (tbuff, sizeof (tbuff), "%s/Start Menu/Programs/Startup/bdj4.bat",
        sysvarsGetStr (SV_HOME));
    filemanipCopy ("install/win-startup.bat", tbuff);
  }

  installer->instState = INST_INST_CLEAN_TMP;
}

static void
installerInstCleanTmp (installer_t *installer)
{
  if (osChangeDir (installer->rundir) < 0) {
    installerFailWorkingDir (installer, installer->rundir, "instcleantmp");
    return;
  }

  instutilInstallCleanTmp (installer->rundir);

  installer->instState = INST_SAVE_LOCALE;
}

static void
installerSaveLocale (installer_t *installer)
{
  char        tbuff [MAXPATHLEN];
  FILE        *fh;

  if (osChangeDir (installer->datatopdir) < 0) {
    installerFailWorkingDir (installer, installer->datatopdir, "savelocale");
    return;
  }

  stpecpy (tbuff, tbuff + sizeof (tbuff), "data/localeorig.txt");
  fh = fileopOpen (tbuff, "w");
  if (fh != NULL) {
    fprintf (fh, "%s\n", sysvarsGetStr (SV_LOCALE_SYSTEM));
    mdextfclose (fh);
    fclose (fh);
  }

  installer->instState = INST_VLC_CHECK;
}

static void
installerVLCCheck (installer_t *installer)
{
  char    tbuff [MAXPATHLEN];

  /* on linux, vlc is installed via other methods */
  /* also on linux, gstreamer can be used even if there is no vlc */
  if (installer->vlcinstalled || isLinux ()) {
    installer->instState = INST_FINALIZE;
    return;
  }

  installerVLCGetVersion (installer);
  if (*installer->vlcversion) {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Downloading %s."), "VLC");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);
    installerDisplayText (installer, INST_DISP_STATUS, installer->pleasewaitmsg, false);
    uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], installer->pleasewaitmsg);
    installer->instState = INST_VLC_DOWNLOAD;
  } else {
    snprintf (tbuff, sizeof (tbuff),
        /* CONTEXT: installer: status message */
        _("Unable to determine %s version."), "VLC");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);

    installer->instState = INST_FINALIZE;
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
        "vlc-%s-win%" PRId64 ".exe",
        installer->vlcversion, sysvarsGetNum (SVL_OS_BITS));
    snprintf (url, sizeof (url),
        "https://get.videolan.org/vlc/last/win%" PRId64 "/%s",
        sysvarsGetNum (SVL_OS_BITS), installer->dlfname);
  }
  if (isMacOS ()) {
    char    *arch;

    /* the os architecture on m1 is 'arm64' and includes the bits */
    /* the os architecture on intel is 'x86_64' */
    arch = sysvarsGetStr (SV_OS_ARCH);
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
    uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], installer->pleasewaitmsg);
    installer->instState = INST_VLC_INSTALL;
  } else {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Download of %s failed."), "VLC");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);

    installer->instState = INST_FINALIZE;
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
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], "");
  installerCheckPackages (installer);

  installer->instState = INST_FINALIZE;
}

static void
installerFinalize (installer_t *installer)
{
  char        tbuff [MAXPATHLEN];

  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], "");

  if (installer->newinstall || installer->reinstall) {
    if (! installer->bdjoptloaded) {
      installerLoadBdjOpt (installer);
    }
    if (installer->bdjoptloaded) {
      instutilGetMusicDir (tbuff, sizeof (tbuff));
      bdjoptSetStr (OPT_M_DIR_MUSIC, tbuff);
      bdjoptSave ();
    }
  }

  if (! installer->readonly) {
    if (! fileopFileExists (sysvarsGetStr (SV_FILE_ALTCOUNT))) {
      FILE    *fh;

      diropMakeDir (sysvarsGetStr (SV_DIR_CONFIG));
      fh = fileopOpen (sysvarsGetStr (SV_FILE_ALTCOUNT), "w");
      if (fh != NULL) {
        fputs ("0\n", fh);
        mdextfclose (fh);
        fclose (fh);
      }
    }

    /* create the new install flag file on a new install */
    if (installer->newinstall) {
      FILE  *fh;

      snprintf (tbuff, sizeof (tbuff), "%s/%s%s", "data",
          NEWINST_FN, BDJ4_CONFIG_EXT);
      fh = fileopOpen (tbuff, "w");
      mdextfclose (fh);
      fclose (fh);
    }
  }

  /* readonly flag */
  if (installer->readonly) {
    FILE  *fh;

    snprintf (tbuff, sizeof (tbuff), "%s/%s%s",
        installer->target, READONLY_FN, BDJ4_CONFIG_EXT);
    fh = fileopOpen (tbuff, "w");
    mdextfclose (fh);
    fclose (fh);
  }

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
    fprintf (stdout, "update %d\n", installer->updateinstall);
    fprintf (stdout, "converted %d\n", installer->convprocess);
    fprintf (stdout, "readonly %d\n", installer->readonly);
    fprintf (stdout, "alternate 0\n");
  }

  if (installer->readonly) {
    installer->instState = INST_FINISH;
  } else {
    installer->instState = INST_UPDATE_PROCESS_INIT;
  }

  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CACHE);
  fileopDelete (tbuff);
  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_FN, BDJ4_LOCK_EXT, PATHBLD_MP_DIR_CACHE);
  fileopDelete (tbuff);
}

static void
installerUpdateProcessInit (installer_t *installer)
{
  char  buff [MAXPATHLEN];

  installer->altidx = 1;

  if (osChangeDir (installer->datatopdir) < 0) {
    installerFailWorkingDir (installer, installer->datatopdir, "updprocessinit");
    return;
  }

  /* the updater must be run in the same locale as the installer */
  if (installer->localespecified) {
    FILE    *fh;
    char    tbuff [MAXPATHLEN];

    stpecpy (tbuff, tbuff + sizeof (tbuff), "data/locale.txt");
    fh = fileopOpen (tbuff, "w");
    fprintf (fh, "%s\n", sysvarsGetStr (SV_LOCALE));
    mdextfclose (fh);
    fclose (fh);
  }

  /* CONTEXT: installer: status message */
  snprintf (buff, sizeof (buff), _("Updating %s."), BDJ4_LONG_NAME);
  installerDisplayText (installer, INST_DISP_ACTION, buff, false);
  installerDisplayText (installer, INST_DISP_STATUS, installer->pleasewaitmsg, false);
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], installer->pleasewaitmsg);
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
    targv [targc++] = "--convert";
  }
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);

  installer->instState = INST_UPDATE_ALT_PROCESS_INIT;
}

static void
installerUpdateAltProcessInit (installer_t *installer)
{
  char  tfn [MAXPATHLEN];
  FILE  *fh;
  char  altdir [MAXPATHLEN];

  if (installer->altidx >= BDJ4_MAX_ALT) {
    installer->instState = INST_REGISTER_INIT;
    return;
  }

  snprintf (tfn, sizeof (tfn), "%s/%s%s%02d%s",
      sysvarsGetStr (SV_DIR_CONFIG), ALT_INST_PATH_FN,
      sysvarsGetStr (SV_BDJ4_DEVELOPMENT), installer->altidx, BDJ4_CONFIG_EXT);
  if (! fileopFileExists (tfn)) {
    installer->instState = INST_UPDATE_ALT_PROCESS_INIT;
    installer->altidx += 1;
    return;
  }

  fh = fileopOpen (tfn, "r");
  if (fh == NULL) {
    installer->instState = INST_UPDATE_ALT_PROCESS_INIT;
    installer->altidx += 1;
    return;
  }

  if (fh != NULL) {
    (void) ! fgets (altdir, sizeof (altdir), fh);
    stringTrim (altdir);
    mdextfclose (fh);
    fclose (fh);
  }

  if (! fileopIsDirectory (altdir)) {
    installer->instState = INST_UPDATE_ALT_PROCESS_INIT;
    installer->altidx += 1;
    return;
  }

  if (osChangeDir (altdir) < 0) {
    installerFailWorkingDir (installer, altdir, "updprocessaltinit");
    installer->altidx += 1;
    return;
  }

  /* the updater must be run in the same locale as the installer */
  if (installer->localespecified) {
    FILE    *fh;
    char    tbuff [MAXPATHLEN];

    stpecpy (tbuff, tbuff + sizeof (tbuff), "data/locale.txt");
    fh = fileopOpen (tbuff, "w");
    fprintf (fh, "%s\n", sysvarsGetStr (SV_LOCALE));
    mdextfclose (fh);
    fclose (fh);
  }

  installer->instState = INST_UPDATE_PROCESS;
  installer->altidx += 1;
}

static void
installerRegisterInit (installer_t *installer)
{
  char    tbuff [200];

  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], "");

  if (osChangeDir (installer->datatopdir) < 0) {
    installerFailWorkingDir (installer, installer->datatopdir, "registerinit");
    return;
  }

  if ((strcmp (sysvarsGetStr (SV_USER), "bll") == 0 ||
      strncmp (sysvarsGetStr (SV_USER), "test ", 5) == 0) &&
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
  char          tbuff [500];

  snprintf (tbuff, sizeof (tbuff),
      "&bdj3version=%s&oldversion=%s"
      "&new=%d&reinstall=%d&update=%d&convert=%d&readonly=%d",
      installer->bdj3version,
      installer->oldversion,
      installer->newinstall,
      installer->reinstall,
      installer->updateinstall,
      installer->convprocess,
      installer->readonly
      );
  instutilRegister (tbuff);
  installer->instState = INST_FINISH;
}

/* support routines */

static void
installerCleanup (installer_t *installer)
{
  /* make sure the installer is not in the bdj4-install dir before */
  /* the clean-inst process is run. */
  /* if installing read-only, there is no data-top-dir */
  /* if the installer was exited without installing, there is no rundir */
  if (osChangeDir (installer->rundir) < 0) {
    char  tbuff [MAXPATHLEN];

    snprintf (tbuff, sizeof (tbuff), "%s/..", installer->unpackdir);
    if (osChangeDir (tbuff) < 0) {
      installerFailWorkingDir (installer, installer->datatopdir, "cleanup");
      return;
    }
  }

  if (installer->bdjoptloaded) {
    bdjoptCleanup ();
  }

  if (installer->guienabled) {
    for (int i = 0; i < INST_W_MAX; ++i) {
      uiwcontFree (installer->wcont [i]);
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

  if (installer->clean && fileopIsDirectory (installer->unpackdir)) {
    char          buff [MAXPATHLEN];

    if (isWindows ()) {
      char          ebuff [MAXPATHLEN];
      size_t        sz = 0;
      char          *fdata;
      char          *ndata;
      FILE          *fh;
      const char    *targv [10];
      int           targc = 0;

      /* create the batch file from a template. */
      /* solves many quoting and startup issues. */

      /* template */
      snprintf (ebuff, sizeof (ebuff), "%s/install/win-clean-inst.bat",
          installer->unpackdir);
      fdata = filedataReadAll (ebuff, &sz);
      if (fdata == NULL) {
        return;
      }

      /* target filename */
      snprintf (ebuff, sizeof (ebuff), "%s/bdj4-clean-inst.bat",
          sysvarsGetStr (SV_DIR_CACHE_BASE));
      fh = fileopOpen (ebuff, "w");
      if (fh == NULL) {
        return;
      }


      ndata = regexReplaceLiteral (installer->unpackdir,
          sysvarsGetStr (SV_USER), WINUSERNAME);
      stpecpy (buff, buff + sizeof (buff), ndata);
      dataFree (ndata);
      pathDisplayPath (buff, strlen (buff));
      ndata = regexReplaceLiteral (fdata, "#UNPACKDIR#", buff);
      fprintf (fh, "%s", ndata);
      mdextfclose (fh);
      fclose (fh);
      dataFree (ndata);
      dataFree (fdata);

      /* target filename */
      pathDisplayPath (ebuff, strlen (ebuff));
      targv [targc++] = ebuff;
      targv [targc++] = NULL;

      osProcessStart (targv,
          OS_PROC_DETACH | OS_PROC_NOSTDERR | OS_PROC_WINDOW_OK, NULL, NULL);
      if (! sysvarsGetNum (SVL_IS_MSYS)) {
        /* give windows a short bit to start the script */
        mssleep (100);
      }
    }
    if (! isWindows ()) {
      /* cleaning up on not-windows is easy */
      diropDeleteDir (installer->unpackdir, DIROP_ALL);
    }
  } else {
    fprintf (stderr, "unpack-dir: %s\n", installer->unpackdir);
  }
}

static void
installerDisplayText (installer_t *installer, const char *pfx,
    const char *txt, bool bold)
{
  if (installer->guienabled) {
    if (bold) {
      uiTextBoxAppendBoldStr (installer->wcont [INST_W_STATUS_DISP], pfx);
      uiTextBoxAppendBoldStr (installer->wcont [INST_W_STATUS_DISP], txt);
      uiTextBoxAppendStr (installer->wcont [INST_W_STATUS_DISP], "\n");
    } else {
      uiTextBoxAppendStr (installer->wcont [INST_W_STATUS_DISP], pfx);
      uiTextBoxAppendStr (installer->wcont [INST_W_STATUS_DISP], txt);
      uiTextBoxAppendStr (installer->wcont [INST_W_STATUS_DISP], "\n");
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
installerGetBDJ3Fname (installer_t *installer, char *buff, size_t sz)
{
  *buff = '\0';
  if (*installer->target) {
    snprintf (buff, sz, "%s%s/install/%s",
        installer->target, installer->macospfx, BDJ3_LOC_FILE);
  }
}

static void
installerSetRundir (installer_t *installer, const char *dir)
{
  installer->rundir [0] = '\0';
  if (*dir) {
    snprintf (installer->rundir, sizeof (installer->rundir),
        "%s%s", dir, installer->macospfx);
    pathNormalizePath (installer->rundir, sizeof (installer->rundir));
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "set rundir: %s", installer->rundir);
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
    /* for the time being, until VLC version 4 is fixed on windows, */
    /* hard-code the downloaded version to 3.0.21 */
    stpecpy (installer->vlcversion,
        installer->vlcversion + sizeof (installer->vlcversion), "3.0.21");
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "https://get.videolan.org/vlc/last/%s/", platform);

  webclientGet (installer->webclient, tbuff);

  if (installer->webresponse != NULL) {
    const char  *srchvlc = "vlc-";

    /* note that all files excepting ".." start with vlc-<version>- */
    /* vlc-<version>.<arch>.dmg{|.asc|.md5|.sha1|.sha256} */
    /* vlc-<version>.<arch>.{7z|exe|msi|zip|debugsys}{|.asc|.md5|.sha1|.sha256} */
    /* vlc-3.0.16-intel64.dmg */
    /* vlc-3.0.17.4-universal.dmg */
    /* vlc-3.0.17.4-win64.exe */
    p = strstr (installer->webresponse, srchvlc);
    if (p != NULL) {
      p += strlen (srchvlc);
      stpecpy (installer->vlcversion,
          installer->vlcversion + sizeof (installer->vlcversion), p);
      e = strstr (installer->vlcversion, "-");
      if (e != NULL) {
        *e = '\0';
      }
    }
  }
}

static void
installerCheckPackages (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];
  char  *tmp;


  tmp = sysvarsGetStr (SV_PATH_VLC);

  if (*tmp) {
    if (installer->guienabled && installer->uiBuilt) {
      /* CONTEXT: installer: display of package status */
      snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "VLC");
      uiLabelSetText (installer->wcont [INST_W_VLC_MSG], tbuff);
    }
    installer->vlcinstalled = true;
  } else {
    if (installer->guienabled && installer->uiBuilt) {
      /* CONTEXT: installer: display of package status */
      snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "VLC");
      uiLabelSetText (installer->wcont [INST_W_VLC_MSG], tbuff);
    }
    installer->vlcinstalled = false;
  }
}

static void
installerWebResponseCallback (void *userdata, const char *resp, size_t len)
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
  installer->aborted = true;
  installer->instState = INST_WAIT_USER;
}

static void
installerSetTargetDir (installer_t *installer, const char *fn)
{
  char        *tmp;
  pathinfo_t  *pi;

  /* fn may be pointing to an allocated value, which is installer->target */
  tmp = mdstrdup (fn);
  dataFree (installer->target);
  installer->target = tmp;
  pathNormalizePath (installer->target, strlen (installer->target));

  pi = pathInfo (installer->target);
  snprintf (installer->name, sizeof (installer->name), "%.*s",
      (int) pi->blen, pi->basename);
  pathInfoFree (pi);

  logMsg (LOG_INSTALL, LOG_IMPORTANT, "set target: %s", installer->target);
}

static void
installerSetBDJ3LocDir (installer_t *installer, const char *fn)
{
  char    *tmp;

  tmp = mdstrdup (fn);
  dataFree (installer->bdj3loc);
  installer->bdj3loc = tmp;
  pathNormalizePath (installer->bdj3loc, strlen (installer->bdj3loc));
}

static void
installerLoadBdjOpt (installer_t *installer)
{
  char        cwd [MAXPATHLEN];

  if (installer->bdjoptloaded) {
    bdjoptCleanup ();
    installer->bdjoptloaded = false;
  }

  osGetCurrentDir (cwd, sizeof (cwd));
  if (osChangeDir (installer->datatopdir) < 0) {
    /* no datatopdir as yet */
    return;
  }

  bdjoptInit ();
  installer->bdjoptloaded = true;

  if (osChangeDir (cwd) < 0) {
    installerFailWorkingDir (installer, installer->datatopdir, "getexistdata");
  }
}

