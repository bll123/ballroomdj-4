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
#include <signal.h>
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

#include "ati.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
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
#include "osprocess.h"
#include "ossignal.h"
#include "osutils.h"
#include "osuiutils.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"
#include "templateutil.h"
#include "ui.h"
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
  INST_SET_ATI,
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
  INST_CB_MUSIC_DIR,
  INST_CB_EXIT,
  INST_CB_INSTALL,
  INST_CB_REINST,
  INST_CB_CONV,
  INST_CB_MAX,
};

enum {
  INST_BUTTON_TARGET_DIR,
  INST_BUTTON_BDJ3LOC_DIR,
  INST_BUTTON_MUSIC_DIR,
  INST_BUTTON_EXIT,
  INST_BUTTON_INSTALL,
  INST_BUTTON_MAX,
};

enum {
  INST_W_WINDOW,
  INST_W_STATUS_MSG,
  INST_W_RE_INSTALL,
  INST_W_FEEDBACK_MSG,
  INST_W_CONVERT,
  INST_W_CONV_FEEDBACK_MSG,
  /* make sure these are in the same order as the INST_ATI_* enums below */
  INST_W_MUSIC_DIR,
  INST_W_VLC_MSG,
  INST_W_PYTHON_MSG,
  INST_W_MUTAGEN_MSG,
  INST_W_MAX,
};

enum {
  INST_TARGET,
  INST_BDJ3LOC,
  INST_MUSICDIR,
};

enum {
  INST_ATI_BDJ4,
  INST_ATI_MUTAGEN,
  INST_ATI_MAX,
};

typedef struct {
  const char  *name;
  bool        needmutagen;
} instati_t;

static instati_t instati [INST_ATI_MAX] = {
  [INST_ATI_BDJ4] = { "libatibdj4", false },
  [INST_ATI_MUTAGEN] = { "libatimutagen", true },
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
  char            ati [40];
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
  uiwcont_t       *wcont [INST_W_MAX];
  uientry_t       *targetEntry;
  uientry_t       *bdj3locEntry;
  uientry_t       *musicdirEntry;
  uitextbox_t     *disptb;
  uibutton_t      *buttons [INST_BUTTON_MAX];
  /* ati */
  char            *musicdir;
  int             atiselect;
  /* flags */
  bool            bdjoptloaded : 1;
  bool            convprocess : 1;
  bool            guienabled : 1;
  bool            inSetConvert : 1;
  bool            newinstall : 1;
  bool            nomutagen : 1;
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
  bool            localespecified : 1;
  bool            clean : 1;
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
static void installerSetMusicDirEntry (installer_t *installer, const char *musicdir);
static bool installerBDJ3LocDirDialog (void *udata);
static bool installerMusicDirDialog (void *udata);
static int  installerValidateTarget (uientry_t *entry, void *udata);
static int  installerValidateProcessTarget (installer_t *installer, const char *dir);
static int  installerValidateBDJ3Loc (uientry_t *entry, void *udata);
static int  installerValidateMusicDir (uientry_t *entry, void *udata);
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
static void installerSetATI (installer_t *installer);
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
static void installerSetrundir (installer_t *installer, const char *dir);
static void installerVLCGetVersion (installer_t *installer);
static void installerPythonGetVersion (installer_t *installer);
static void installerCheckPackages (installer_t *installer);
static void installerWebResponseCallback (void *userdata, char *resp, size_t len);
static void installerFailWorkingDir (installer_t *installer, const char *dir, const char *msg);
static void installerSetTargetDir (installer_t *installer, const char *fn);
static void installerSetBDJ3LocDir (installer_t *installer, const char *fn);
static void installerSetMusicDir (installer_t *installer, const char *fn);
static void installerCheckAndFixTarget (char *buff, size_t sz);
static void installerSetATISelect (installer_t *installer);
static void installerGetExistingData (installer_t *installer);
static void installerScanMusicDir (installer_t *installer);

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
    { "ati",        required_argument,  NULL,   'A' },
    { "bdj3dir",    required_argument,  NULL,   '3' },
    { "bdj4installer",no_argument,      NULL,   0 },
    { "locale",     required_argument,  NULL,   'L' },
    { "musicdir",   required_argument,  NULL,   'm' },
    { "noclean",    no_argument,        NULL,   'c' },
    { "nodatafiles", no_argument,       NULL,   'N' },
    { "nomutagen",  no_argument,        NULL,   'M' },
    { "reinstall",  no_argument,        NULL,   'r' },
    { "targetdir",  required_argument,  NULL,   't' },
    { "testregistration", no_argument,  NULL,   'T' },
    { "unattended", no_argument,        NULL,   'U' },
    { "unpackdir",  required_argument,  NULL,   'u' },
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
    { "origcwd",      required_argument,  NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };


#if BDJ4_MEM_DEBUG
  mdebugInit ("inst");
#endif

  buff [0] = '\0';

  installer.unpackdir [0] = '\0';
  installer.home = NULL;
  installer.instState = INST_INITIALIZE;
  installer.lastInstState = INST_INITIALIZE;
  installer.target = mdstrdup ("");
  installer.rundir [0] = '\0';
  installer.bdj3loc = mdstrdup ("");
  installer.musicdir = mdstrdup ("");
  installer.convidx = 0;
  installer.convlist = NULL;
  installer.tclshloc = NULL;
  installer.currdir [0] = '\0';
  installer.disptb = NULL;
  /* CONTEXT: installer: status message */
  installer.pleasewaitmsg = _("Please wait\xe2\x80\xa6");

  installer.bdjoptloaded = false;
  installer.convprocess = false;
  installer.guienabled = true;
  installer.inSetConvert = false;
  installer.newinstall = true;
  installer.nomutagen = false;
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
  installer.localespecified = false;
  installer.clean = true;
  for (int i = 0; i < INST_CB_MAX; ++i) {
    installer.callbacks [i] = NULL;
  }
  for (int i = 0; i < INST_W_MAX; ++i) {
    installer.wcont [i] = NULL;
  }

  installer.loglevel = LOG_IMPORTANT | LOG_BASIC | LOG_INFO | LOG_REDIR_INST;
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
  installer.musicdirEntry = NULL;

  /* the data in sysvars will not be correct.  don't use it.  */
  /* the installer only needs the home, hostname, os info and locale */
  sysvarsInit (argv[0]);
  localeInit ();

  strlcpy (installer.ati, instati [INST_ATI_BDJ4].name, sizeof (installer.ati));
  installerSetATISelect (&installer);

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

  instutilGetMusicDir (buff, sizeof (buff));
  dataFree (installer.musicdir);
  installer.musicdir = mdstrdup (buff);

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
      case 'c': {
        installer.clean = false;
        break;
      }
      case 'u': {
        strlcpy (installer.unpackdir, optarg, sizeof (installer.unpackdir));
        break;
      }
      case 'A': {
        strlcpy (installer.ati, optarg, sizeof (installer.ati));
        installerSetATISelect (&installer);
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
      case 'L': {
        sysvarsSetStr (SV_LOCALE, optarg);
        snprintf (tbuff, sizeof (tbuff), "%.2s", optarg);
        sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
        sysvarsSetNum (SVL_LOCALE_SET, 1);
        installer.localespecified = true;
        break;
      }
      case 'm': {
        dataFree (installer.musicdir);
        installer.musicdir = mdstrdup (optarg);
        break;
      }
      case 'M': {
        installer.nomutagen = true;
        break;
      }
      case 'N': {
        installer.nodatafiles = true;
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
    installer.musicdirEntry = uiEntryInit (60, MAXPATHLEN);
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
      dataFree (installer.musicdir);
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
    dataFree (installer.musicdir);
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

  pathDisplayPath (installer.target, strlen (installer.target));

  if (installer.guienabled) {
    char *uifont;

    uiUIInitialize ();

    uifont = sysvarsGetStr (SV_FONT_DEFAULT);
    if (uifont == NULL || ! *uifont) {
      uifont = "Arial Regular 11";
      if (isMacOS ()) {
        uifont = "Arial Regular 17";
      }
    }
    uiSetUICSS (uifont, INST_HL_COLOR, NULL);
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
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uibutton_t    *uibutton;
  uiwcont_t     *uiwidgetp;
  uiwcont_t     *szgrp;
  char          tbuff [100];
  char          imgbuff [MAXPATHLEN];

  szgrp = uiCreateSizeGroupHoriz ();

  uiLabelAddClass (INST_HL_CLASS, INST_HL_COLOR);
  uiSeparatorAddClass (INST_SEP_CLASS, INST_SEP_COLOR);

  strlcpy (imgbuff, "img/bdj4_icon_inst.png", sizeof (imgbuff));
  osuiSetIcon (imgbuff);

  strlcpy (imgbuff, "img/bdj4_icon_inst.svg", sizeof (imgbuff));
  /* CONTEXT: installer: window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Installer"), BDJ4_NAME);
  installer->callbacks [INST_CB_EXIT] = callbackInit (
      installerExitCallback, installer, NULL);
  installer->wcont [INST_W_WINDOW] = uiCreateMainWindow (
      installer->callbacks [INST_CB_EXIT],
      tbuff, imgbuff);
  uiWindowSetDefaultSize (installer->wcont [INST_W_WINDOW], 1000, 600);

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiBoxPackInWindow (installer->wcont [INST_W_WINDOW], vbox);

  /* begin line : status message */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  installer->wcont [INST_W_STATUS_MSG] = uiCreateLabel ("");
  uiWidgetAlignHorizEnd (installer->wcont [INST_W_STATUS_MSG]);
  uiWidgetSetClass (installer->wcont [INST_W_STATUS_MSG], INST_HL_CLASS);
  uiBoxPackEndExpand (hbox, installer->wcont [INST_W_STATUS_MSG]);

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

  uiEntryCreate (installer->targetEntry);
  uiEntrySetValue (installer->targetEntry, installer->target);
  uiwidgetp = uiEntryGetWidgetContainer (installer->targetEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiEntrySetValidate (installer->targetEntry,
      installerValidateTarget, installer, UIENTRY_DELAYED);

  installer->callbacks [INST_CB_TARGET_DIR] = callbackInit (
      installerTargetDirDialog, installer, NULL);
  uibutton = uiCreateButton (
      installer->callbacks [INST_CB_TARGET_DIR],
      "", NULL);
  installer->buttons [INST_BUTTON_TARGET_DIR] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);

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
      installerCheckDirTarget, installer, NULL);
  uiToggleButtonSetCallback (installer->wcont [INST_W_RE_INSTALL],
      installer->callbacks [INST_CB_REINST]);

  installer->wcont [INST_W_FEEDBACK_MSG] = uiCreateLabel ("");
  uiWidgetSetClass (installer->wcont [INST_W_FEEDBACK_MSG], INST_HL_CLASS);
  uiBoxPackStart (hbox, installer->wcont [INST_W_FEEDBACK_MSG]);

  uiwcontFree (hbox);

  /* begin line : music dir */
  /* the music dir is scanned in order to set the audio tag interface */
  /* appropriately */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: installation: the music folder where the user stores their music */
  uiwidgetp = uiCreateColonLabel (_("Music Folder"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiEntryCreate (installer->musicdirEntry);
  installerSetMusicDirEntry (installer, installer->musicdir);
  uiwidgetp = uiEntryGetWidgetContainer (installer->musicdirEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiEntrySetValidate (installer->musicdirEntry,
      installerValidateMusicDir, installer, UIENTRY_DELAYED);

  installer->callbacks [INST_CB_MUSIC_DIR] = callbackInit (
      installerMusicDirDialog, installer, NULL);
  uibutton = uiCreateButton (
      installer->callbacks [INST_CB_MUSIC_DIR],
      "", NULL);
  installer->buttons [INST_BUTTON_MUSIC_DIR] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);

  /* begin line : separator */
  uiwidgetp = uiCreateHorizSeparator ();
  uiWidgetSetMarginTop (uiwidgetp, 2);
  uiWidgetSetMarginBottom (uiwidgetp, 2);
  uiWidgetSetClass (uiwidgetp, INST_SEP_CLASS);
  uiBoxPackStart (vbox, uiwidgetp);
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

  uiEntryCreate (installer->bdj3locEntry);
  installerSetBDJ3LocEntry (installer, installer->bdj3loc);
  uiwidgetp = uiEntryGetWidgetContainer (installer->bdj3locEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiEntrySetValidate (installer->bdj3locEntry,
      installerValidateBDJ3Loc, installer, UIENTRY_DELAYED);

  installer->callbacks [INST_CB_BDJ3LOC_DIR] = callbackInit (
      installerBDJ3LocDirDialog, installer, NULL);
  uibutton = uiCreateButton (
      installer->callbacks [INST_CB_BDJ3LOC_DIR],
      "", NULL);
  installer->buttons [INST_BUTTON_BDJ3LOC_DIR] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);

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
      installerCheckDirConv, installer, NULL);
  uiToggleButtonSetCallback (installer->wcont [INST_W_CONVERT],
      installer->callbacks [INST_CB_CONV]);

  installer->wcont [INST_W_CONV_FEEDBACK_MSG] = uiCreateLabel ("");
  uiWidgetSetClass (installer->wcont [INST_W_CONV_FEEDBACK_MSG], INST_HL_CLASS);
  uiBoxPackStart (hbox, installer->wcont [INST_W_CONV_FEEDBACK_MSG]);

  uiwcontFree (hbox);

  /* begin line : separator */
  uiwidgetp = uiCreateHorizSeparator ();
  uiWidgetSetMarginTop (uiwidgetp, 2);
  uiWidgetSetMarginBottom (uiwidgetp, 2);
  uiWidgetSetClass (uiwidgetp, INST_SEP_CLASS);
  uiBoxPackStart (vbox, uiwidgetp);
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
  uiWidgetSetClass (installer->wcont [INST_W_VLC_MSG], INST_HL_CLASS);
  uiBoxPackStart (hbox, installer->wcont [INST_W_VLC_MSG]);

  uiwcontFree (hbox);

  /* begin line : python message */
  /* python status */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel ("Python");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  installer->wcont [INST_W_PYTHON_MSG] = uiCreateLabel ("");
  uiWidgetSetClass (installer->wcont [INST_W_PYTHON_MSG], INST_HL_CLASS);
  uiBoxPackStart (hbox, installer->wcont [INST_W_PYTHON_MSG]);

  uiwcontFree (hbox);

  /* begin line : mutagen message */
  /* mutagen status */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel ("Mutagen");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  installer->wcont [INST_W_MUTAGEN_MSG] = uiCreateLabel ("");
  uiWidgetSetClass (installer->wcont [INST_W_MUTAGEN_MSG], INST_HL_CLASS);
  uiBoxPackStart (hbox, installer->wcont [INST_W_MUTAGEN_MSG]);

  uiwcontFree (hbox);

  /* begin line : buttons */
  /* button box */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uibutton = uiCreateButton (
      installer->callbacks [INST_CB_EXIT],
      /* CONTEXT: installer: exits the installer */
      _("Exit"), NULL);
  installer->buttons [INST_BUTTON_EXIT] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackEnd (hbox, uiwidgetp);

  installer->callbacks [INST_CB_INSTALL] = callbackInit (
      installerInstallCallback, installer, NULL);
  uibutton = uiCreateButton (
      installer->callbacks [INST_CB_INSTALL],
      /* CONTEXT: installer: start the installation process */
      _("Install"), NULL);
  installer->buttons [INST_BUTTON_INSTALL] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackEnd (hbox, uiwidgetp);

  installer->disptb = uiTextBoxCreate (250, INST_HL_COLOR);
  uiTextBoxSetReadonly (installer->disptb);
  uiTextBoxHorizExpand (installer->disptb);
  uiTextBoxVertExpand (installer->disptb);
  uiBoxPackStartExpand (vbox, uiTextBoxGetScrolledWindow (installer->disptb));

  uiWidgetShowAll (installer->wcont [INST_W_WINDOW]);
  installer->uiBuilt = true;

  /* call this again to set the radio buttons */
  installerSetATISelect (installer);

  uiwcontFree (hbox);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);
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
  uiEntryValidate (installer->musicdirEntry, false);

  if (installer->guienabled && installer->scrolltoend) {
    uiTextBoxScrollToEnd (installer->disptb);
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
      installerDisplayConvert (installer);
      installerCheckPackages (installer);

      uiEntryValidate (installer->targetEntry, true);
      uiEntryValidate (installer->bdj3locEntry, true);
      uiEntryValidate (installer->musicdirEntry, true);
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
    case INST_SET_ATI: {
      installerSetATI (installer);
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

  return true;
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
  tbool = uiToggleButtonIsActive (installer->wcont [INST_W_RE_INSTALL]);
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
        uiLabelSetText (installer->wcont [INST_W_FEEDBACK_MSG], tbuff);
        installerSetConvert (installer, UI_TOGGLE_BUTTON_ON);
      } else {
        /* CONTEXT: installer: message indicating the action that will be taken */
        snprintf (tbuff, sizeof (tbuff), _("Updating existing %s installation."), BDJ4_NAME);
        uiLabelSetText (installer->wcont [INST_W_FEEDBACK_MSG], tbuff);
        installerSetConvert (installer, UI_TOGGLE_BUTTON_OFF);
      }
    } else {
      /* CONTEXT: installer: the selected folder exists and is not a BDJ4 installation */
      uiLabelSetText (installer->wcont [INST_W_FEEDBACK_MSG], _("Error: Folder already exists."));
      installerSetConvert (installer, UI_TOGGLE_BUTTON_OFF);
      rc = UIENTRY_ERROR;
    }
  }
  if (! exists) {
    /* CONTEXT: installer: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("New %s installation."), BDJ4_NAME);
    uiLabelSetText (installer->wcont [INST_W_FEEDBACK_MSG], tbuff);
    installerSetConvert (installer, UI_TOGGLE_BUTTON_ON);
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
  pathNormalizePath (tbuff, strlen (tbuff));
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
    uiLabelSetText (installer->wcont [INST_W_CONV_FEEDBACK_MSG], tbuff);
    installerSetConvert (installer, UI_TOGGLE_BUTTON_OFF);
  }

  /* will call display-convert */
  installerSetPaths (installer);
  return rc;
}

static int
installerValidateMusicDir (uientry_t *entry, void *udata)
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

  /* music dir validation */

  strlcpy (tbuff, uiEntryGetValue (installer->musicdirEntry), sizeof (tbuff));
  pathNormalizePath (tbuff, strlen (tbuff));
  if (*tbuff &&
      fileopIsDirectory (tbuff)) {
    locok = true;
  }

  if (! locok) {
    rc = UIENTRY_ERROR;
    /* CONTEXT: installer: the location entered is not a valid music folder. */
    snprintf (tbuff, sizeof (tbuff), _("Not a valid music folder."));
    uiLabelSetText (installer->wcont [INST_W_CONV_FEEDBACK_MSG], tbuff);
    installerSetConvert (installer, UI_TOGGLE_BUTTON_OFF);
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

  selectdata = uiDialogCreateSelect (installer->wcont [INST_W_WINDOW],
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
  pathDisplayPath (tbuff, sizeof (tbuff));
  uiEntrySetValue (installer->bdj3locEntry, tbuff);
}

static void
installerSetMusicDirEntry (installer_t *installer, const char *musicdir)
{
  char    tbuff [MAXPATHLEN];

  strlcpy (tbuff, musicdir, sizeof (tbuff));
  pathDisplayPath (tbuff, sizeof (tbuff));
  uiEntrySetValue (installer->musicdirEntry, tbuff);
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
  selectdata = uiDialogCreateSelect (installer->wcont [INST_W_WINDOW],
      tbuff, uiEntryGetValue (installer->bdj3locEntry), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    installerSetBDJ3LocEntry (installer, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected bdj3 loc: %s", installer->bdj3loc);
    mdfree (fn);
  }
  mdfree (selectdata);
  return UICB_CONT;
}

static bool
installerMusicDirDialog (void *udata)
{
  installer_t *installer = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;
  char        tbuff [100];

  /* CONTEXT: installer: music folder selection dialog: window title */
  snprintf (tbuff, sizeof (tbuff), _("Select Music Folder Location"));
  selectdata = uiDialogCreateSelect (installer->wcont [INST_W_WINDOW],
      tbuff, uiEntryGetValue (installer->musicdirEntry), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    installerSetMusicDirEntry (installer, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected music dir: %s", installer->musicdir);
    mdfree (fn);
  }
  mdfree (selectdata);
  return UICB_CONT;
}

static void
installerSetConvert (installer_t *installer, int state)
{
  installer->inSetConvert = true;
  uiToggleButtonSetState (installer->wcont [INST_W_CONVERT], state);
  installer->inSetConvert = false;
}

static void
installerDisplayConvert (installer_t *installer)
{
  int           nval;
  char          *tptr;
  char          tbuff [200];
  bool          nodir = false;

  nval = uiToggleButtonIsActive (installer->wcont [INST_W_CONVERT]);

  if (strcmp (installer->bdj3loc, "-") == 0 ||
      *installer->bdj3loc == '\0' ||
      installer->nodatafiles) {
    nval = false;
    nodir = true;
    installerSetConvert (installer, UI_TOGGLE_BUTTON_OFF);
  }

  installer->convprocess = nval;

  if (nval) {
    /* CONTEXT: installer: message indicating the conversion action that will be taken */
    tptr = _("%s data will be converted.");
    snprintf (tbuff, sizeof (tbuff), tptr, BDJ3_NAME);
    uiLabelSetText (installer->wcont [INST_W_CONV_FEEDBACK_MSG], tbuff);
  } else {
    if (nodir) {
      /* CONTEXT: installer: message indicating the conversion action that will be taken */
      tptr = _("No folder specified.");
      uiLabelSetText (installer->wcont [INST_W_CONV_FEEDBACK_MSG], tptr);
    } else {
      /* CONTEXT: installer: message indicating the conversion action that will be taken */
      tptr = _("The conversion process will not be run.");
      uiLabelSetText (installer->wcont [INST_W_CONV_FEEDBACK_MSG], tptr);
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

  strlcpy (installer->datatopdir, installer->rundir, MAXPATHLEN);
  if (isMacOS ()) {
    snprintf (installer->datatopdir, MAXPATHLEN,
        "%s/Library/Application Support/BDJ4",
        installer->home);
  }

  if (exists) {
    installerGetExistingData (installer);
  }
  return exists;
}

static void
installerSetPaths (installer_t *installer)
{
  /* the target dir should already be set by the validation process */
  installerSetBDJ3LocDir (installer, uiEntryGetValue (installer->bdj3locEntry));
  installerSetMusicDir (installer, uiEntryGetValue (installer->musicdirEntry));
  installerDisplayConvert (installer);
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
    /* verification on windows is too slow */
    strlcpy (tmp, "OK", sizeof (tmp));
  } else {
    targv [0] = "./install/verifychksum.sh";
    if (isMacOS ()) {
      targv [0] = "./Contents/MacOS/install/verifychksum.sh";
    }
    targv [1] = NULL;
    osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, tmp, sizeof (tmp), NULL);
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
installerInstInit (installer_t *installer)
{
  bool        exists = false;
  bool        found = false;
  char        tbuff [MAXPATHLEN];

  if (installer->guienabled) {
    installerSetPaths (installer);

    installer->reinstall = uiToggleButtonIsActive (installer->wcont [INST_W_RE_INSTALL]);
    if (installer->newinstall) {
      installer->reinstall = false;
    }
  }

  if (! installer->guienabled && ! installer->unattended) {
    /* target directory */

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

      /* music directory */

      tbuff [0] = '\0';
      /* CONTEXT: installer: command line interface: asking for the user's music folder */
      printf (_("Enter the music folder."));
      printf ("\n");
      /* CONTEXT: installer: command line interface: instructions */
      printf (_("Press 'Enter' to select the default."));
      printf ("\n");
      printf ("[%s] : ", installer->musicdir);
      fflush (stdout);
      (void) ! fgets (tbuff, sizeof (tbuff), stdin);
      stringTrim (tbuff);
      if (*tbuff != '\0') {
        dataFree (installer->musicdir);
        installer->musicdir = mdstrdup (tbuff);
      }

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
        uiLabelSetText (installer->wcont [INST_W_FEEDBACK_MSG], _("Folder already exists."));
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
    if (strcmp (rlvl, "production") == 0) {
      rlvl = "";
    }
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
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], installer->pleasewaitmsg);

  /* the unpackdir is not necessarily the same as the rundir */
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
    pathDisplayPath (tmp, sizeof (tmp));
    snprintf (tbuff, sizeof (tbuff),
        "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl . \"%s\"",
        tmp);
  } else {
    snprintf (tbuff, sizeof (tbuff), "tar -c -f - . | (cd '%s'; tar -x -f -)",
        installer->target);
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "copy files: %s", tbuff);
  (void) ! system (tbuff);
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], "");
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
  if (! installer->newinstall) {
    installerGetExistingData (installer);
  }

  if (! installer->newinstall && ! installer->reinstall) {
    installer->instState = INST_CONVERT_START;
    return;
  }

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Creating folder structure."), false);

  /* this will create the directories necessary for the configs */
  /* namely: profile00, <hostname>, <hostname>/profile00 */
  bdjoptCreateDirectories ();
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

  if (chdir (installer->datatopdir)) {
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
    filemanipLinkCopy (from, to);

    snprintf (from, sizeof (from), "../Applications/BDJ4.app/Contents/MacOS/plocal/share/themes/Mojave-light-solid");
    snprintf (to, sizeof (to), "%s/.themes/Mojave-light-solid", installer->home);
    filemanipLinkCopy (from, to);
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
  snprintf (tbuff, sizeof (tbuff), "%s/%s/%" PRId64 "/tcl/bin/tclsh",
      installer->bdj3loc, sysvarsGetStr (SV_OSNAME), sysvarsGetNum (SVL_OSBITS));
  locs [locidx++] = mdstrdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/Applications/BallroomDJ.app/Contents/%s/%" PRId64 "/tcl/bin/tclsh",
      installer->home, sysvarsGetStr (SV_OSNAME), sysvarsGetNum (SVL_OSBITS));
  locs [locidx++] = mdstrdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/local/bin/tclsh", installer->home);
  locs [locidx++] = mdstrdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/bin/tclsh", installer->home);
  locs [locidx++] = mdstrdup (tbuff);
  /* for testing; low priority */
  snprintf (tbuff, sizeof (tbuff), "%s/../%s/%" PRId64 "/tcl/bin/tclsh",
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
  installerGetExistingData (installer);

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_STATUS, _("Conversion complete."), false);
  installer->instState = INST_CREATE_SHORTCUT;
}

static void
installerCreateShortcut (installer_t *installer)
{
  if (chdir (installer->rundir)) {
    installerFailWorkingDir (installer, installer->rundir, "createshortcut");
    return;
  }

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, INST_DISP_ACTION, _("Creating shortcut."), false);

  /* handles linux and windows desktop shortcut */
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

  installer->instState = INST_SET_ATI;
}

static void
installerSetATI (installer_t *installer)
{
  if (chdir (installer->datatopdir)) {
    installerFailWorkingDir (installer, installer->rundir, "createshortcut");
    return;
  }

  if (! installer->bdjoptloaded) {
    /* the audio tag interface must be saved */
    bdjoptInit ();
    installer->bdjoptloaded = true;
  }

  if (installer->newinstall || installer->reinstall) {
    /* CONTEXT: installer: status message */
    installerDisplayText (installer, INST_DISP_ACTION, _("Scanning music folder."), false);

    /* this will scan the music dir and set the audio tag interface */
    installerScanMusicDir (installer);
  }

  if (installer->bdjoptloaded) {
    bdjoptSetStr (OPT_M_DIR_MUSIC, installer->musicdir);
    bdjoptSetStr (OPT_M_AUDIOTAG_INTFC, installer->ati);
    bdjoptSave ();
  }
  installer->instState = INST_VLC_CHECK;
}

static void
installerVLCCheck (installer_t *installer)
{
  char    tbuff [MAXPATHLEN];

  /* on linux, vlc is installed via other methods */
  if (installer->vlcinstalled || isLinux ()) {
    if (instati [installer->atiselect].needmutagen) {
      installer->instState = INST_PYTHON_CHECK;
    } else {
      installer->instState = INST_UPDATE_PROCESS_INIT;
    }
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

    if (instati [installer->atiselect].needmutagen) {
      installer->instState = INST_PYTHON_CHECK;
    } else {
      installer->instState = INST_UPDATE_PROCESS_INIT;
    }
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
        installer->vlcversion, sysvarsGetNum (SVL_OSBITS));
    snprintf (url, sizeof (url),
        "https://get.videolan.org/vlc/last/win%" PRId64 "/%s",
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
    uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], installer->pleasewaitmsg);
    installer->instState = INST_VLC_INSTALL;
  } else {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Download of %s failed."), "VLC");
    installerDisplayText (installer, INST_DISP_ACTION, tbuff, false);

    if (instati [installer->atiselect].needmutagen) {
      installer->instState = INST_PYTHON_CHECK;
    } else {
      installer->instState = INST_UPDATE_PROCESS_INIT;
    }
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

  if (instati [installer->atiselect].needmutagen) {
    installer->instState = INST_PYTHON_CHECK;
  } else {
    installer->instState = INST_UPDATE_PROCESS_INIT;
  }
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
    uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], installer->pleasewaitmsg);
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
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], "");
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

  /* must appear after the nodatafiles check */
  if (installer->nomutagen) {
    installer->instState = INST_UPDATE_PROCESS_INIT;
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
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], installer->pleasewaitmsg);
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
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], "");
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

  if (chdir (installer->datatopdir)) {
    installerFailWorkingDir (installer, installer->datatopdir, "copytemplates");
    return;
  }

  /* the updater must be run in the same locale as the installer */
  if (installer->localespecified) {
    FILE    *fh;
    char    tbuff [512];

    strlcpy (tbuff, "data/locale.txt", sizeof (tbuff));
    fh = fileopOpen (tbuff, "w");
    fprintf (fh, "%s\n", sysvarsGetStr (SV_LOCALE));
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
    targv [targc++] = "--converted";
  }
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
  uiLabelSetText (installer->wcont [INST_W_STATUS_MSG], "");

  installer->instState = INST_FINALIZE;
}

static void
installerFinalize (installer_t *installer)
{
  char    tbuff [MAXPATHLEN];

  /* clear the python version cache files */
  snprintf (tbuff, sizeof (tbuff), "%s/%s%s", "data",
      SYSVARS_PY_DOT_VERS_FN, BDJ4_CONFIG_EXT);
  fileopDelete (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/%s%s", "data",
      SYSVARS_PY_VERS_FN, BDJ4_CONFIG_EXT);
  fileopDelete (tbuff);

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

  if (installer->bdjoptloaded) {
    bdjoptCleanup ();
  }

  if (installer->guienabled) {
    uiEntryFree (installer->targetEntry);
    uiEntryFree (installer->bdj3locEntry);
    uiTextBoxFree (installer->disptb);
    for (int i = 0; i < INST_BUTTON_MAX; ++i) {
      uiButtonFree (installer->buttons [i]);
    }
    for (int i = 0; i < INST_W_MAX; ++i) {
      uiwcontFree (installer->wcont [i]);
    }
  }
  for (int i = 0; i < INST_CB_MAX; ++i) {
    callbackFree (installer->callbacks [i]);
  }
  dataFree (installer->target);
  dataFree (installer->bdj3loc);
  dataFree (installer->musicdir);
  slistFree (installer->convlist);
  dataFree (installer->tclshloc);

  webclientClose (installer->webclient);

  if (! fileopIsDirectory (installer->unpackdir)) {
    return;
  }

  if (installer->clean) {
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
installerSetrundir (installer_t *installer, const char *dir)
{
  installer->rundir [0] = '\0';
  if (*dir) {
    strlcpy (installer->rundir, dir, sizeof (installer->rundir));
    if (isMacOS ()) {
      strlcat (installer->rundir, MACOS_PREFIX, sizeof (installer->rundir));
    }
    pathNormalizePath (installer->rundir, sizeof (installer->rundir));
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

  if (instati [installer->atiselect].needmutagen == false) {
    /* CONTEXT: installer: display of package status */
    snprintf (tbuff, sizeof (tbuff), _("Not needed"));
    uiLabelSetText (installer->wcont [INST_W_PYTHON_MSG], tbuff);
    uiLabelSetText (installer->wcont [INST_W_MUTAGEN_MSG], tbuff);
  } else {
    sysvarsGetPythonVersion ();
    sysvarsCheckMutagen ();

    tmp = sysvarsGetStr (SV_PATH_PYTHON);

    if (*tmp) {
      if (installer->guienabled && installer->uiBuilt) {
        /* CONTEXT: installer: display of package status */
        snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "Python");
        uiLabelSetText (installer->wcont [INST_W_PYTHON_MSG], tbuff);
      }
      installer->pythoninstalled = true;
    } else {
      if (installer->guienabled && installer->uiBuilt) {
        /* CONTEXT: installer: display of package status */
        snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Python");
        uiLabelSetText (installer->wcont [INST_W_PYTHON_MSG], tbuff);
        /* CONTEXT: installer: display of package status */
        snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Mutagen");
        uiLabelSetText (installer->wcont [INST_W_MUTAGEN_MSG], tbuff);
      }
      installer->pythoninstalled = false;
    }

    if (installer->pythoninstalled) {
      tmp = sysvarsGetStr (SV_PYTHON_MUTAGEN);
      if (installer->guienabled && installer->uiBuilt) {
        if (*tmp) {
          /* CONTEXT: installer: display of package status */
          snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "Mutagen");
          uiLabelSetText (installer->wcont [INST_W_MUTAGEN_MSG], tbuff);
        } else {
          /* CONTEXT: installer: display of package status */
          snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Mutagen");
          uiLabelSetText (installer->wcont [INST_W_MUTAGEN_MSG], tbuff);
        }
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
  pathNormalizePath (installer->target, strlen (installer->target));
}

static void
installerSetBDJ3LocDir (installer_t *installer, const char *fn)
{
  dataFree (installer->bdj3loc);
  installer->bdj3loc = mdstrdup (fn);
  pathNormalizePath (installer->bdj3loc, strlen (installer->bdj3loc));
}

static void
installerSetMusicDir (installer_t *installer, const char *fn)
{
  dataFree (installer->musicdir);
  installer->musicdir = mdstrdup (fn);
  pathNormalizePath (installer->musicdir, strlen (installer->musicdir));
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

static void
installerSetATISelect (installer_t *installer)
{
  for (int i = 0; i < INST_ATI_MAX; ++i) {
    if (strcmp (installer->ati, instati [i].name) == 0) {
      installer->atiselect = i;
      break;
    }
  }
  installerCheckPackages (installer);
}

static void
installerGetExistingData (installer_t *installer)
{
  const char  *tmp = NULL;
  char        cwd [MAXPATHLEN];

  if (installer->bdjoptloaded) {
    bdjoptCleanup ();
    installer->bdjoptloaded = false;
  }

  if (installer->newinstall) {
    return;
  }

  (void) ! getcwd (cwd, sizeof (cwd));
  if (chdir (installer->datatopdir)) {
    return;
  }

  bdjoptInit ();
  installer->bdjoptloaded = true;

  tmp = bdjoptGetStr (OPT_M_DIR_MUSIC);
  if (tmp != NULL) {
    dataFree (installer->musicdir);
    installer->musicdir = mdstrdup (tmp);
    installerSetMusicDirEntry (installer, installer->musicdir);
  }
  tmp = bdjoptGetStr (OPT_M_AUDIOTAG_INTFC);
  if (tmp != NULL) {
    strlcpy (installer->ati, tmp, sizeof (installer->ati));
    installerSetATISelect (installer);
  }

  if (chdir (cwd)) {
    installerFailWorkingDir (installer, installer->datatopdir, "get-old-b");
  }
}

/* scan the music directory and determine which */
/* ati interface has the best support */
/* prefer bdj4 internal ati where possible */
static void
installerScanMusicDir (installer_t *installer)
{
  slist_t     *mlist;
  slistidx_t  iteridx;
  const char  *fn;
  char        tbuff [MAXPATHLEN];
  int         tagtype;
  int         filetype;
  int         flags [AFILE_TYPE_MAX];
  int         supported [INST_ATI_MAX][AFILE_TYPE_MAX];
  int         suppval [INST_ATI_MAX];
  int         max;
  int         idx;

  for (int j = 0; j < AFILE_TYPE_MAX; ++j) {
    flags [j] = 0;
  }

  mlist = dirlistRecursiveDirList (installer->musicdir, DIRLIST_FILES);
  slistStartIterator (mlist, &iteridx);
  while ((fn = slistIterateKey (mlist, &iteridx)) != NULL) {
    audiotagDetermineTagType (fn, &tagtype, &filetype);
    flags [filetype] = 1;
  }
  slistFree (mlist);

  /* recall that sysvars is not set up correctly */
  /* pathbld_mp_dir_exec must be set properly */
  /* the dynamically loaded libraries need this path */
  snprintf (tbuff, sizeof (tbuff), "%s/bin", installer->rundir);
  sysvarsSetStr (SV_BDJ4_DIR_EXEC, tbuff);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "dir-exec set to %s", tbuff);

  for (int i = 0; i < INST_ATI_MAX; ++i) {
    atiGetSupportedTypes (instati [i].name, supported [i]);
  }

  for (int i = 0; i < INST_ATI_MAX; ++i) {
    suppval [i] = 0;

    for (int j = 0; j < AFILE_TYPE_MAX; ++j) {
      /* don't worry about unknown types */
      if (j == AFILE_TYPE_UNKNOWN) {
        continue;
      }
      if (flags [j] > 0) {
        if (supported [i][j] == ATI_READ_WRITE) {
          suppval [i] += 2;
        }
        if (supported [i][j] == ATI_READ) {
          suppval [i] += 1;
        }
      }
    }
  }

  /* want bdj4 internal ati if possible */
  idx = INST_ATI_BDJ4;
  max = suppval [idx];
  for (int i = 0; i < INST_ATI_MAX; ++i) {
    if (suppval [i] > max) {
      max = suppval [i];
      idx = i;
    }
  }

  strlcpy (installer->ati, instati [idx].name, sizeof (installer->ati));
  installerSetATISelect (installer);
}
