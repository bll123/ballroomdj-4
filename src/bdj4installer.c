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
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "datafile.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "fileutil.h"
#include "localeutil.h"
#include "locatebdj3.h"
#include "log.h"
#include "osutils.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "webclient.h"

/* installation states */
typedef enum {
  INST_BEGIN,
  INST_INIT,
  INST_SAVE,
  INST_MAKE_TARGET,
  INST_COPY_START,
  INST_COPY_FILES,
  INST_CHDIR,
  INST_CREATE_DIRS,
  INST_COPY_TEMPLATES,
  INST_CONVERT_START,
  INST_CONVERT,
  INST_CONVERT_WAIT,
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
  INST_UPDATE_CA_FILE_INIT,
  INST_UPDATE_CA_FILE,
  INST_UPDATE_PROCESS_INIT,
  INST_UPDATE_PROCESS,
  INST_REGISTER_INIT,
  INST_REGISTER,
  INST_FINISH,
  INST_EXIT,
} installstate_t;

enum {
  INST_CALLBACK_SELECT_DIR,
  INST_CALLBACK_EXIT,
  INST_CALLBACK_INSTALL,
  INST_CALLBACK_MAX,
};

enum {
  INST_TARGET,
  INST_BDJ3LOC,
};

typedef struct {
  installstate_t  instState;
  UICallback      callbacks [INST_CALLBACK_MAX];
  char            *home;
  char            *target;
  char            hostname [MAXPATHLEN];
  char            locale [40];
  char            rundir [MAXPATHLEN];
  char            datatopdir [MAXPATHLEN];
  char            currdir [MAXPATHLEN];
  char            unpackdir [MAXPATHLEN];
  char            vlcversion [40];
  char            pyversion [40];
  char            dlfname [MAXPATHLEN];
  char            oldversion [MAXPATHLEN];
  char            bdj3version [MAXPATHLEN];
  webclient_t     *webclient;
  char            *webresponse;
  size_t          webresplen;
  /* conversion */
  char            *bdj3loc;
  char            *tclshloc;
  slist_t         *convlist;
  slistidx_t      convidx;
  uientry_t       targetEntry;
  uientry_t       bdj3locEntry;
  UIWidget        window;
  UIWidget        feedbackMsg;
  UIWidget        convFeedbackMsg;
  UIWidget        vlcMsg;
  UIWidget        pythonMsg;
  UIWidget        mutagenMsg;
  UIWidget        reinstWidget;
  UICallback      reinstcb;
  UIWidget        convWidget;
  UICallback      convcb;
  uitextbox_t     *disptb;
  /* flags */
  bool            newinstall : 1;
  bool            reinstall : 1;
  bool            convprocess : 1;
  bool            freetarget : 1;
  bool            freebdj3loc : 1;
  bool            guienabled : 1;
  bool            vlcinstalled : 1;
  bool            pythoninstalled : 1;
  bool            inSetConvert : 1;
  bool            uiBuilt : 1;
  bool            scrolltoend : 1;
} installer_t;

#define INST_HL_COLOR "#b16400"
#define INST_SEP_COLOR "#733000"

#define INST_NEW_FILE "data/newinstall.txt"
#define INST_TEMP_FILE  "tmp/bdj4instout.txt"
#define INST_SAVE_FNAME "installdir.txt"
#define CONV_TEMP_FILE "tmp/bdj4convout.txt"
#define BDJ3_LOC_FILE "install/bdj3loc.txt"

static void installerBuildUI (installer_t *installer);
static int  installerMainLoop (void *udata);
static bool installerExitCallback (void *udata);
static bool installerCheckDir (void *udata);
static bool installerSelectDirDialog (void *udata);
static int  installerValidateTarget (uientry_t *entry, void *udata);
static int  installerValidateBDJ3Loc (uientry_t *entry, void *udata);
static void installerSetConvert (installer_t *installer, int val);
static void installerDisplayConvert (installer_t *installer);
static bool installerInstallCallback (void *udata);
static bool installerCheckTarget (installer_t *installer, const char *dir);
static void installerSetPaths (installer_t *installer);

static void installerInstInit (installer_t *installer);
static void installerSaveTargetDir (installer_t *installer);
static void installerMakeTarget (installer_t *installer);
static void installerCopyStart (installer_t *installer);
static void installerCopyFiles (installer_t *installer);
static void installerChangeDir (installer_t *installer);
static void installerCreateDirs (installer_t *installer);
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
static void installerUpdateCAFileInit (installer_t *installer);
static void installerUpdateCAFile (installer_t *installer);
static void installerRegisterInit (installer_t *installer);
static void installerRegister (installer_t *installer);

static void installerCleanup (installer_t *installer);
static void installerDisplayText (installer_t *installer, char *pfx, char *txt);
static void installerGetTargetSaveFname (installer_t *installer, char *buff, size_t len);
static void installerGetBDJ3Fname (installer_t *installer, char *buff, size_t len);
static void installerTemplateCopy (const char *dir, const char *from, const char *to);
static void installerSetrundir (installer_t *installer, const char *dir);
static void installerVLCGetVersion (installer_t *installer);
static void installerPythonGetVersion (installer_t *installer);
static void installerCheckPackages (installer_t *installer);
static void installerWebResponseCallback (void *userdata, char *resp, size_t len);

int
main (int argc, char *argv[])
{
  installer_t   installer;
  char          tbuff [512];
  char          buff [MAXPATHLEN];
  char          bdj3buff [MAXPATHLEN];
  FILE          *fh;
  int           c = 0;
  int           option_index = 0;

  static struct option bdj_options [] = {
    { "installer",  no_argument,        NULL,   0 },
    { "reinstall",  no_argument,        NULL,   'r' },
    { "guidisabled",no_argument,        NULL,   'g' },
    { "unpackdir",  required_argument,  NULL,   'u' },
    { "debug",      required_argument,  NULL,   0 },
    { "theme",      required_argument,  NULL,   0 },
    { "debugself",  no_argument,        NULL,   0 },
    { "bdj4",       no_argument,        NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };


  bdj3buff [0] = '\0';
  installer.unpackdir [0] = '\0';
  buff [0] = '\0';
  installer.home = getenv ("HOME");
  if (installer.home == NULL) {
    /* probably a windows machine */
    installer.home = getenv ("USERPROFILE");
  }

  uiutilsUIWidgetInit (&installer.window);
  installer.instState = INST_BEGIN;
  installer.target = buff;
  installer.rundir [0] = '\0';
  installer.locale [0] = '\0';
  installer.bdj3loc = bdj3buff;
  installer.convidx = 0;
  installer.convlist = NULL;
  installer.tclshloc = NULL;
  installer.currdir [0] = '\0';
  installer.newinstall = true;
  installer.reinstall = false;
  installer.convprocess = false;
  installer.freetarget = false;
  installer.freebdj3loc = false;
  installer.guienabled = true;
  installer.vlcinstalled = false;
  installer.pythoninstalled = false;
  installer.inSetConvert = false;
  installer.uiBuilt = false;
  installer.scrolltoend = false;
  uiutilsUIWidgetInit (&installer.reinstWidget);
  uiutilsUIWidgetInit (&installer.feedbackMsg);
  uiutilsUIWidgetInit (&installer.convFeedbackMsg);
  uiutilsUIWidgetInit (&installer.vlcMsg);
  uiutilsUIWidgetInit (&installer.pythonMsg);
  uiutilsUIWidgetInit (&installer.mutagenMsg);
  uiutilsUIWidgetInit (&installer.convWidget);
  getcwd (installer.currdir, sizeof (installer.currdir));
  installer.webclient = NULL;
  strcpy (installer.vlcversion, "");
  strcpy (installer.pyversion, "");
  strcpy (installer.oldversion, "");
  strcpy (installer.bdj3version, "");

  uiEntryInit (&installer.targetEntry, 80, MAXPATHLEN);
  uiEntryInit (&installer.bdj3locEntry, 80, MAXPATHLEN);

  while ((c = getopt_long_only (argc, argv, "g:r:u:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'g': {
        installer.guienabled = false;
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
      default: {
        break;
      }
    }
  }

  if (*installer.unpackdir == '\0') {
    fprintf (stderr, "Error: unpackdir argument is required\n");
    exit (1);
  }

  snprintf (tbuff, sizeof (tbuff), "%s/BDJ4", installer.home);
  strlcpy (buff, tbuff, sizeof (buff));
  if (isMacOS ()) {
    snprintf (buff, sizeof (buff), "%s/Applications/BDJ4.app", installer.home);
  }

  /* the data in sysvars will not be correct.  don't use it.  */
  /* the installer only needs the hostname, os info and locale */
  sysvarsInit (argv[0]);
  localeInit ();

  strlcpy (installer.hostname, sysvarsGetStr (SV_HOSTNAME),
      sizeof (installer.hostname));

  installerGetTargetSaveFname (&installer, tbuff, sizeof (tbuff));
  fh = fileopOpen (tbuff, "r");
  if (fh != NULL) {
    /* installer.target is pointing at buff */
    fgets (buff, sizeof (buff), fh);
    stringTrim (buff);
    fclose (fh);
  }

  /* target is not set, choose a proper default */
  if (! *installer.target) {
    char  *home;

    home = sysvarsGetStr (SV_HOME);
    snprintf (buff, sizeof (buff), "%s/%s", home, BDJ4_NAME);
  }

  /* this only works if the installer.target is pointing at an existing */
  /* install of BDJ4 */
  installerGetBDJ3Fname (&installer, tbuff, sizeof (tbuff));
  fh = fileopOpen (tbuff, "r");
  if (fh != NULL) {
    /* installer.bdj3loc is pointing at bdj3buff */
    fgets (bdj3buff, sizeof (bdj3buff), fh);
    stringTrim (bdj3buff);
    fclose (fh);
  } else {
    installer.bdj3loc = locatebdj3 ();
    installer.freebdj3loc = true;
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "initial bdj3loc: %s", installer.bdj3loc);

  if (isWindows ()) {
    /* installer.target is pointing at buff */
    pathWinPath (installer.target, sizeof (buff));
  }

  if (installer.guienabled) {
    char *uifont;

    uiUIInitialize ();

    uifont = sysvarsGetStr (SV_FONT_DEFAULT);
    if (uifont == NULL || ! *uifont) {
      uifont = "Arial 12";
    }
    uiSetUIFont (uifont);
  }

  installerCheckPackages (&installer);

  if (installer.guienabled) {
    installerBuildUI (&installer);
  } else {
    installer.instState = INST_INIT;
  }

  while (installer.instState != INST_EXIT) {
    installerMainLoop (&installer);
    mssleep (10);
  }

  /* process any final events */
  if (installer.guienabled) {
    uiUIProcessEvents ();
  }

  installerCleanup (&installer);
  logEnd ();
  return 0;
}

static void
installerBuildUI (installer_t *installer)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      *uiwidgetp;
  UIWidget      sg;
  char          tbuff [100];
  char          imgbuff [MAXPATHLEN];


  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&uiwidget);

  strlcpy (imgbuff, "img/bdj4_icon_inst.svg", sizeof (imgbuff));
  /* CONTEXT: installer: window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Installer"), BDJ4_NAME);
  uiutilsUICallbackInit (&installer->callbacks [INST_CALLBACK_EXIT],
      installerExitCallback, installer);
  uiCreateMainWindow (&installer->window,
      &installer->callbacks [INST_CALLBACK_EXIT],
      tbuff, imgbuff);
  uiWindowSetDefaultSize (&installer->window, 1000, 600);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 10);
  uiWidgetSetMarginTop (&vbox, 20);
  uiWidgetExpandHoriz (&vbox);
  uiWidgetExpandVert (&vbox);
  uiBoxPackInWindow (&installer->window, &vbox);

  uiCreateLabel (&uiwidget,
      /* CONTEXT: installer: where BDJ4 gets installed */
      _("Enter the destination folder where BDJ4 will be installed."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiEntryCreate (&installer->targetEntry);
  uiEntrySetValue (&installer->targetEntry, installer->target);
  uiwidgetp = uiEntryGetUIWidget (&installer->targetEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStart (&vbox, uiwidgetp);
  uiEntrySetValidate (&installer->targetEntry,
      installerValidateTarget, installer);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: installer: checkbox: overwrite the previous BDJ4 installation */
  uiCreateCheckButton (&installer->reinstWidget, _("Overwrite"),
      installer->reinstall);
  uiBoxPackStart (&hbox, &installer->reinstWidget);
  uiutilsUICallbackInit (&installer->reinstcb, installerCheckDir, installer);
  uiToggleButtonSetCallback (&installer->reinstWidget, &installer->reinstcb);

  uiCreateLabel (&installer->feedbackMsg, "");
  uiLabelSetColor (&installer->feedbackMsg, INST_HL_COLOR);
  uiBoxPackStart (&hbox, &installer->feedbackMsg);

  uiCreateHorizSeparator (&uiwidget);
  uiSeparatorSetColor (&uiwidget, INST_SEP_COLOR);
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

  uiEntryCreate (&installer->bdj3locEntry);
  uiEntrySetValue (&installer->bdj3locEntry, installer->bdj3loc);
  uiwidgetp = uiEntryGetUIWidget (&installer->bdj3locEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiEntrySetValidate (&installer->bdj3locEntry,
      installerValidateBDJ3Loc, installer);

  uiutilsUICallbackInit (&installer->callbacks [INST_CALLBACK_SELECT_DIR],
      installerSelectDirDialog, installer);
  uiCreateButton (&uiwidget,
      &installer->callbacks [INST_CALLBACK_SELECT_DIR],
      "", NULL, NULL, NULL);
  uiButtonSetImageIcon (&uiwidget, "folder");
  uiWidgetSetMarginStart (&uiwidget, 0);
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: installer: checkbox: convert the BallroomDJ 3 installation */
  snprintf (tbuff, sizeof (tbuff), _("Convert %s"), BDJ3_NAME);
  uiCreateCheckButton (&installer->convWidget, tbuff, 0);
  uiBoxPackStart (&hbox, &installer->convWidget);
  uiutilsUICallbackInit (&installer->convcb, installerCheckDir, installer);
  uiToggleButtonSetCallback (&installer->convWidget, &installer->convcb);

  uiCreateLabel (&installer->convFeedbackMsg, "");
  uiLabelSetColor (&installer->convFeedbackMsg, INST_HL_COLOR);
  uiBoxPackStart (&hbox, &installer->convFeedbackMsg);

  /* VLC status */

  uiCreateHorizSeparator (&uiwidget);
  uiSeparatorSetColor (&uiwidget, INST_SEP_COLOR);
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateSizeGroupHoriz (&sg);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget, "VLC");
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiCreateLabel (&installer->vlcMsg, "");
  uiLabelSetColor (&installer->vlcMsg, INST_HL_COLOR);
  uiBoxPackStart (&hbox, &installer->vlcMsg);

  /* python status */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget, "Python");
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiCreateLabel (&installer->pythonMsg, "");
  uiLabelSetColor (&installer->pythonMsg, INST_HL_COLOR);
  uiBoxPackStart (&hbox, &installer->pythonMsg);

  /* mutagen status */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget, "Mutagen");
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiCreateLabel (&installer->mutagenMsg, "");
  uiLabelSetColor (&installer->mutagenMsg, INST_HL_COLOR);
  uiBoxPackStart (&hbox, &installer->mutagenMsg);

  /* button box */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateButton (&uiwidget,
      &installer->callbacks [INST_CALLBACK_EXIT],
      /* CONTEXT: exits the installer */
      _("Exit"), NULL, NULL, NULL);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiutilsUICallbackInit (&installer->callbacks [INST_CALLBACK_SELECT_DIR],
      installerInstallCallback, installer);
  uiCreateButton (&uiwidget,
      &installer->callbacks [INST_CALLBACK_SELECT_DIR],
      /* CONTEXT: installer: start the installation process */
      _("Install"), NULL, NULL, NULL);
  uiBoxPackEnd (&hbox, &uiwidget);

  installer->disptb = uiTextBoxCreate (400);
  uiTextBoxSetReadonly (installer->disptb);
  uiTextBoxHorizExpand (installer->disptb);
  uiTextBoxVertExpand (installer->disptb);
  uiBoxPackStartExpand (&vbox, uiTextBoxGetScrolledWindow (installer->disptb));

  uiWidgetShowAll (&installer->window);
  installer->uiBuilt = true;

  installerDisplayConvert (installer);
  installerCheckPackages (installer);
}

static int
installerMainLoop (void *udata)
{
  installer_t *installer = udata;

  uiEntryValidate (&installer->targetEntry, false);
  uiEntryValidate (&installer->bdj3locEntry, false);

  if (installer->guienabled) {
    uiUIProcessEvents ();
  }

  if (installer->guienabled && installer->scrolltoend) {
    uiTextBoxScrollToEnd (installer->disptb);
    installer->scrolltoend = false;
    uiUIProcessEvents ();
    mssleep (10);
    uiUIProcessEvents ();
    /* go through the main loop once more */
    return TRUE;
  }

  switch (installer->instState) {
    case INST_BEGIN: {
      /* do nothing */
      break;
    }
    case INST_INIT: {
      installerInstInit (installer);
      break;
    }
    case INST_SAVE: {
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
    case INST_CHDIR: {
      installerChangeDir (installer);
      break;
    }
    case INST_CREATE_DIRS: {
      installerCreateDirs (installer);

      logStartAppend ("bdj4installer", "in",
          LOG_IMPORTANT | LOG_BASIC | LOG_MAIN);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "=== installer started");
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "target: %s", installer->target);
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
    case INST_CONVERT_WAIT: {
      /* do nothing */
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
    case INST_UPDATE_CA_FILE_INIT: {
      installerUpdateCAFileInit (installer);
      break;
    }
    case INST_UPDATE_CA_FILE: {
      installerUpdateCAFile (installer);
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
      installerDisplayText (installer, "## ",  _("Installation complete."));
      if (installer->guienabled) {
        installer->instState = INST_BEGIN;
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
installerCheckDir (void *udata)
{
  installer_t   *installer = udata;

  uiEntryValidate (&installer->targetEntry, true);
  uiEntryValidate (&installer->bdj3locEntry, true);
  return UICB_CONT;
}

static int
installerValidateTarget (uientry_t *entry, void *udata)
{
  installer_t   *installer = udata;
  const char    *dir;
  bool          exists = false;
  bool          changed = false;
  bool          tbool;
  char          tbuff [100];

  if (! installer->guienabled) {
    return UIENTRY_ERROR;
  }

  if (! installer->uiBuilt) {
    return UIENTRY_RESET;
  }

  uiLabelSetText (&installer->feedbackMsg, "");

  dir = uiEntryGetValue (&installer->targetEntry);
  tbool = uiToggleButtonIsActive (&installer->reinstWidget);
  if (tbool != installer->reinstall) {
    changed = true;
  }
  installer->reinstall = tbool;
  uiLabelSetText (&installer->feedbackMsg, "");

  exists = fileopIsDirectory (dir);
  if (exists) {
    exists = installerCheckTarget (installer, dir);
    if (exists) {
      if (installer->reinstall) {
        /* CONTEXT: installer: message indicating the action that will be taken */
        snprintf (tbuff, sizeof (tbuff), _("Overwriting existing %s installation."), BDJ4_NAME);
        uiLabelSetText (&installer->feedbackMsg, tbuff);
        if (changed) {
          installerSetConvert (installer, TRUE);
        }
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
    }
  } else {
    /* CONTEXT: installer: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("New %s installation."), BDJ4_NAME);
    uiLabelSetText (&installer->feedbackMsg, tbuff);
    installerSetConvert (installer, TRUE);
  }

  return UIENTRY_OK;
}

static int
installerValidateBDJ3Loc (uientry_t *entry, void *udata)
{
  installer_t   *installer = udata;
  bool          locok = false;
  const char    *fn;
  int           rc;

  if (! installer->guienabled) {
    return UIENTRY_ERROR;
  }

  if (! installer->uiBuilt) {
    return UIENTRY_RESET;
  }

  installerDisplayConvert (installer);

  /* bdj3 location validation */

  if (! installer->convprocess) {
    return UIENTRY_OK;
  }

  fn = uiEntryGetValue (&installer->bdj3locEntry);
  if (*fn == '\0' || strcmp (fn, "-") == 0 || locationcheck (fn)) {
    locok = true;
  }

  if (! locok) {
    rc = UIENTRY_ERROR;
    installerSetConvert (installer, FALSE);
  } else {
    rc = UIENTRY_OK;
  }

  installerSetPaths (installer);
  return rc;
}

static bool
installerSelectDirDialog (void *udata)
{
  installer_t     *installer = udata;
  char            *fn = NULL;
  uiselect_t selectdata;
  char            tbuff [100];

  /* CONTEXT: installer: label for entry field for BDJ3 location */
  snprintf (tbuff, sizeof (tbuff), _("Select %s Location"), BDJ3_NAME);
  selectdata.label = tbuff;
  selectdata.window = &installer->window;
  selectdata.startpath = uiEntryGetValue (&installer->bdj3locEntry);
  fn = uiSelectDirDialog (&selectdata);
  if (fn != NULL) {
    if (installer->bdj3loc != NULL && installer->freebdj3loc) {
      free (installer->bdj3loc);
    }
    installer->bdj3loc = fn;
    installer->freebdj3loc = true;
    uiEntrySetValue (&installer->bdj3locEntry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", installer->bdj3loc);
  }
  return UICB_CONT;
}

static void
installerSetConvert (installer_t *installer, int val)
{
  uiToggleButtonSetState (&installer->convWidget, val);
}

static void
installerDisplayConvert (installer_t *installer)
{
  int           nval;
  char          *tptr;

  nval = uiToggleButtonIsActive (&installer->convWidget);

  installer->inSetConvert = true;

  if (strcmp (installer->bdj3loc, "-") == 0 ||
      *installer->bdj3loc == '\0') {
    nval = 0;
    uiToggleButtonSetState (&installer->convWidget, nval);
  }

  installer->inSetConvert = false;
  installer->convprocess = nval;

  if (nval) {
    /* CONTEXT: installer: message indicating the conversion action that will be taken */
    tptr = _("Conversion will be processed");
    uiLabelSetText (&installer->convFeedbackMsg, tptr);
  } else {
    /* CONTEXT: installer: message indicating the conversion action that will be taken */
    tptr = _("No conversion.");
    uiLabelSetText (&installer->convFeedbackMsg, tptr);
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

  if (installer->instState == INST_BEGIN) {
    installer->instState = INST_INIT;
  }
  return UICB_CONT;
}

static bool
installerCheckTarget (installer_t *installer, const char *dir)
{
  char        tbuff [MAXPATHLEN];
  bool        exists;

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
  if (installer->target != NULL && installer->freetarget) {
    free (installer->target);
  }
  installer->target = strdup (uiEntryGetValue (&installer->targetEntry));
  installer->freetarget = true;

  if (installer->bdj3loc != NULL && installer->freebdj3loc) {
    free (installer->bdj3loc);
  }
  installer->bdj3loc = strdup (uiEntryGetValue (&installer->bdj3locEntry));
  installer->freebdj3loc = true;
}


/* installation routines */

static void
installerInstInit (installer_t *installer)
{
  bool        exists = false;
  char        tbuff [MAXPATHLEN];

  if (installer->guienabled) {
    installerSetPaths (installer);
  }

  if (installer->guienabled) {
    installer->reinstall = uiToggleButtonIsActive (&installer->reinstWidget);
  }

  if (! installer->guienabled) {
    tbuff [0] = '\0';
    /* CONTEXT: installer: command line interface: asking for the BDJ4 destination */
    printf (_("Enter the destination folder."));
    printf ("\n");
    /* CONTEXT: installer: command line interface: instructions */
    printf (_("Press 'Enter' to select the default."));
    printf ("\n");
    printf ("[%s] : ", installer->target);
    fflush (stdout);
    fgets (tbuff, sizeof (tbuff), stdin);
    stringTrim (tbuff);
    if (*tbuff != '\0') {
      if (installer->target != NULL && *installer->target) {
        free (installer->target);
      }
      installer->target = strdup (tbuff);
    }
  }

  exists = fileopIsDirectory (installer->target);
  if (exists) {
    exists = installerCheckTarget (installer, installer->target);

    if (exists && ! installer->guienabled) {
      printf ("\n");
      if (installer->reinstall) {
        /* CONTEXT: installer: command line interface: indicating action */
        printf (_("Overwriting existing %s installation."), BDJ4_NAME);
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
      fgets (tbuff, sizeof (tbuff), stdin);
      stringTrim (tbuff);
      if (*tbuff != '\0') {
        if (strncmp (tbuff, "Y", 1) != 0 &&
            strncmp (tbuff, "y", 1) != 0) {
          /* CONTEXT: installer: command line interface: status message */
          printf (" * %s", _("Installation aborted."));
          printf ("\n");
          fflush (stdout);
          installer->instState = INST_BEGIN;
          return;
        }
      }
    }

    if (! exists) {
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
      installerDisplayText (installer, "", tbuff);
      /* CONTEXT: installer: command line interface: status message */
      installerDisplayText (installer, " * ", _("Installation aborted."));
      installer->instState = INST_BEGIN;
      return;
    }
  }

  installerSetrundir (installer, installer->target);
  installer->instState = INST_SAVE;
}

static void
installerSaveTargetDir (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];
  FILE  *fh;

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Saving install location."));

  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "%s/AppData/Roaming/BDJ4", installer->home);
  } else {
    snprintf (tbuff, sizeof (tbuff), "%s/.config/BDJ4", installer->home);
  }
  fileopMakeDir (tbuff);

  installerGetTargetSaveFname (installer, tbuff, sizeof (tbuff));

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

  fileopMakeDir (installer->target);
  fileopMakeDir (installer->rundir);

  *installer->oldversion = '\0';
  snprintf (tbuff, sizeof (tbuff), "%s/VERSION.txt",
      installer->target);
  if (fileopFileExists (tbuff)) {
    char *nm, *ver, *build, *bdate, *rlvl;

    data = filedataReadAll (tbuff, NULL);
    tp = strtok_r (data, "\r\n", &tokptr);
    while (tp != NULL) {
      nm = strtok_r (tp, "=", &tokptrb);
      p = strtok_r (NULL, "=", &tokptrb);
      if (strcmp (nm, "VERSION") == 0) {
        ver = p;
      }
      if (strcmp (nm, "BUILD") == 0) {
        build = p;
      }
      if (strcmp (nm, "BUILDDATE") == 0) {
        bdate = p;
      }
      if (strcmp (nm, "RELEASELEVEL") == 0) {
        rlvl = p;
      }
      stringTrim (p);
      tp = strtok_r (NULL, "\r\n", &tokptr);
    }
    strlcat (installer->oldversion, ver, sizeof (installer->oldversion));
    strlcat (installer->oldversion, "-", sizeof (installer->oldversion));
    if (*rlvl) {
      strlcat (installer->oldversion, rlvl, sizeof (installer->oldversion));
      strlcat (installer->oldversion, "-", sizeof (installer->oldversion));
    }
    strlcat (installer->oldversion, bdate, sizeof (installer->oldversion));
    strlcat (installer->oldversion, "-", sizeof (installer->oldversion));
    strlcat (installer->oldversion, build, sizeof (installer->oldversion));
    free (data);
  }

  installer->instState = INST_COPY_START;
}

static void
installerCopyStart (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Copying files."));
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "   ", _("Please wait..."));

  /* the unpackdir is not necessarily the same as the current dir */
  /* on mac os, they are different */
  if (chdir (installer->unpackdir) < 0) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->unpackdir);
    /* CONTEXT: installer: failure message */
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    /* CONTEXT: installer: status message */
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  installer->instState = INST_COPY_FILES;
}

static void
installerCopyFiles (installer_t *installer)
{
  char      tbuff [MAXPATHLEN];

  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl . \"%s\"",
        installer->rundir);
    system (tbuff);
  } else {
    snprintf (tbuff, sizeof (tbuff), "tar -c -f - . | (cd '%s'; tar -x -f -)",
        installer->target);
    system (tbuff);
  }
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "   ", _("Copy finished."));
  installer->instState = INST_CHDIR;
}

static void
installerChangeDir (installer_t *installer)
{
  strlcpy (installer->datatopdir, installer->rundir, MAXPATHLEN);
  if (isMacOS ()) {
    snprintf (installer->datatopdir, MAXPATHLEN,
        "%s/Library/Application Support/BDJ4",
        installer->home);
  }

  fileopMakeDir (installer->datatopdir);

  if (chdir (installer->datatopdir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->datatopdir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  installer->instState = INST_CREATE_DIRS;
}

static void
installerCreateDirs (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Creating folder structure."));

  /* create the directories that are not included in the distribution */
  fileopMakeDir ("data");
  /* this will create the directories necessary for the configs */
  bdjoptCreateDirectories ();
  fileopMakeDir ("tmp");
  fileopMakeDir ("http");

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


  if (! installer->newinstall && ! installer->reinstall) {
    installer->instState = INST_CONVERT_START;
    return;
  }

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Copying template files."));

  if (chdir (installer->datatopdir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->datatopdir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  renamelist = NULL;

  snprintf (tbuff, sizeof (tbuff), "%s/install/%s",
      installer->rundir, "localized-sr.txt");
  srdf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);
  snprintf (tbuff, sizeof (tbuff), "%s/install/%s",
      installer->rundir, "localized-auto.txt");
  autodf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);
  snprintf (tbuff, sizeof (tbuff), "%s/install/%s",
      installer->rundir, "localized-qd.txt");
  qddf = datafileAllocParse ("loc-qd", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);

  snprintf (dir, sizeof (dir), "%s/templates", installer->rundir);

  snprintf (tbuff, sizeof (tbuff), "%s/templates", installer->rundir);
  dirlist = filemanipBasicDirList (tbuff, NULL);
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
      free (pi);
      continue;
    }

    if (pathInfoExtCheck (pi, ".crt")) {
      snprintf (from, sizeof (from), "%s", fname);
      snprintf (to, sizeof (to), "http/%s", fname);
    } else if (pathInfoExtCheck (pi, ".svg")) {
      snprintf (from, sizeof (from), "%s", fname);
      snprintf (to, sizeof (to), "%s/img/%s",
          installer->rundir, fname);
    } else if (strncmp (fname, "bdjconfig", 9) == 0) {
      snprintf (from, sizeof (from), "%s", fname);

      snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->blen, pi->basename);
      if (pathInfoExtCheck (pi, ".g")) {
        snprintf (to, sizeof (to), "data/%s", tbuff);
      } else if (pathInfoExtCheck (pi, ".p")) {
        snprintf (to, sizeof (to), "data/profile00/%s", tbuff);
      } else if (pathInfoExtCheck (pi, ".m")) {
        snprintf (to, sizeof (to), "data/%s/%s", installer->hostname, tbuff);
      } else if (pathInfoExtCheck (pi, ".mp")) {
        snprintf (to, sizeof (to), "data/%s/profile00/%s",
            installer->hostname, tbuff);
      } else {
        /* one of the localized versions */
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
          snprintf (tbuff, sizeof (tbuff), "%s%*s", tval, (int) pi->elen,
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
      free (pi);
      continue;
    }

    installerTemplateCopy (dir, from, to);

    free (pi);
  }
  slistFree (dirlist);

  snprintf (dir, sizeof (dir), "%s/img", installer->rundir);

  strlcpy (from, "favicon.ico", sizeof (from));
  snprintf (to, sizeof (to), "http/favicon.ico");
  installerTemplateCopy (dir, from, to);

  strlcpy (from, "led_on.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/led_on.svg");
  installerTemplateCopy (dir, from, to);

  strlcpy (from, "led_off.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/led_off.svg");
  installerTemplateCopy (dir, from, to);

  strlcpy (from, "ballroomdj4.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/ballroomdj4.svg");
  installerTemplateCopy (dir, from, to);

  snprintf (from, sizeof (from), "%s/img/mrc", installer->rundir);
  snprintf (to, sizeof (to), "http/mrc");
  *tbuff = '\0';
  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl \"%s\" \"%s\"",
        from, to);
  } else {
    snprintf (tbuff, sizeof (tbuff), "cp -r '%s' '%s'", from, "http");
  }
  system (tbuff);

  if (isMacOS ()) {
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


  if (! installer->convprocess) {
    installer->instState = INST_CREATE_SHORTCUT;
    return;
  }

  if (strcmp (installer->bdj3loc, "-") == 0 ||
     *installer->bdj3loc == '\0') {
    installer->instState = INST_CREATE_SHORTCUT;
    return;
  }

  if (! installer->guienabled) {
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
    fgets (tbuff, sizeof (tbuff), stdin);
    stringTrim (tbuff);
    if (*tbuff != '\0') {
      if (installer->bdj3loc != NULL && installer->freebdj3loc) {
        free (installer->bdj3loc);
      }
      installer->bdj3loc = strdup (tbuff);
      installer->freebdj3loc = true;
    }
  }

  if (strcmp (installer->bdj3loc, "-") == 0 ||
     *installer->bdj3loc == '\0') {
    installer->instState = INST_CREATE_SHORTCUT;
    return;
  }

  if (chdir (installer->rundir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->rundir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  *installer->bdj3version = '\0';
  snprintf (tbuff, sizeof (tbuff), "%s/VERSION.txt",
      installer->bdj3loc);
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
    free (data);
  }

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Starting conversion process."));

  installer->convlist = filemanipBasicDirList ("conv", ".tcl");
  /* the sort order doesn't matter, but there's a need to run */
  /* a check after everything has finished to make sure the user's */
  /* organization path is in orgopt.txt */
  /* having a sorted list makes it easy to name something to run last */
  slistSort (installer->convlist);
  slistStartIterator (installer->convlist, &installer->convidx);

  locidx = 0;
  snprintf (tbuff, sizeof (tbuff), "%s/%s/%zd/tcl/bin/tclsh",
      installer->bdj3loc, sysvarsGetStr (SV_OSNAME), sysvarsGetNum (SVL_OSBITS));
  locs [locidx++] = strdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/Applications/BallroomDJ.app/Contents/%s/%zd/tcl/bin/tclsh",
      installer->home, sysvarsGetStr (SV_OSNAME), sysvarsGetNum (SVL_OSBITS));
  locs [locidx++] = strdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/local/bin/tclsh", installer->home);
  locs [locidx++] = strdup (tbuff);
  snprintf (tbuff, sizeof (tbuff), "%s/bin/tclsh", installer->home);
  locs [locidx++] = strdup (tbuff);
  /* for testing; low priority */
  snprintf (tbuff, sizeof (tbuff), "%s/../%s/%zd/tcl/bin/tclsh",
      installer->bdj3loc, sysvarsGetStr (SV_OSNAME), sysvarsGetNum (SVL_OSBITS));
  locs [locidx++] = strdup (tbuff);
  locs [locidx++] = strdup ("/opt/local/bin/tclsh");
  locs [locidx++] = strdup ("/usr/local/bin/tclsh");
  locs [locidx++] = strdup ("/usr/bin/tclsh");
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
      installer->tclshloc = strdup (tbuff);
      /* CONTEXT: installer: status message */
      installerDisplayText (installer, "   ", _("Located 'tclsh'."));
    }

    free (locs [locidx]);
    ++locidx;
  }

  if (installer->tclshloc == NULL) {
    /* CONTEXT: installer: failure message */
    snprintf (tbuff, sizeof (tbuff), _("Unable to locate %s."), "tclsh");
    installerDisplayText (installer, "   ", tbuff);
    /* CONTEXT: installer: status message */
    installerDisplayText (installer, "   ", _("Skipping conversion."));
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
  char      *targv [15];

  fn = slistIterateKey (installer->convlist, &installer->convidx);
  if (fn == NULL) {
    installer->instState = INST_CONVERT_FINISH;
    return;
  }

  /* CONTEXT: installer: status message */
  snprintf (buffa, sizeof (buffa), _("Running conversion script: %s."), fn);
  installerDisplayText (installer, "   ", buffa);

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
  installerDisplayText (installer, "   ", _("Conversion complete."));
  installer->instState = INST_CREATE_SHORTCUT;
}

static void
installerCreateShortcut (installer_t *installer)
{
  char buff [MAXPATHLEN];

  if (chdir (installer->rundir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->rundir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Creating shortcut."));
  if (isWindows ()) {
    if (! chdir ("install")) {
      snprintf (buff, sizeof (buff), ".\\makeshortcut.bat \"%s\"",
          installer->rundir);
      system (buff);
      chdir (installer->rundir);
    }
  }
  if (isMacOS ()) {
#if _lib_symlink
    /* on macos, the startup program must be a gui program, otherwise */
    /* the dock icon is not correct */
    /* this must exist and match the name of the app */
    symlink ("bin/bdj4g", "BDJ4");
    /* desktop shortcut */
    snprintf (buff, sizeof (buff), "%s/Desktop/BDJ4.app", installer->home);
    symlink (installer->target, buff);
#endif
  }
  if (isLinux ()) {
    snprintf (buff, sizeof (buff), "./install/linuxshortcut.sh '%s'",
        installer->rundir);
    system (buff);
  }

  installer->instState = INST_VLC_CHECK;
}

static void
installerVLCCheck (installer_t *installer)
{
  char    tbuff [MAXPATHLEN];

  if (installer->vlcinstalled) {
    installer->instState = INST_PYTHON_CHECK;
    return;
  }

  if (chdir (installer->datatopdir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->datatopdir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  installerVLCGetVersion (installer);
  if (*installer->vlcversion) {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Downloading %s."), "VLC");
    installerDisplayText (installer, "-- ", tbuff);
    installer->instState = INST_VLC_DOWNLOAD;
  } else {
    snprintf (tbuff, sizeof (tbuff),
        /* CONTEXT: installer: status message */
        _("Unable to determine %s version."), "VLC");
    installerDisplayText (installer, "-- ", tbuff);
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
        "vlc-%s-win%zd.exe",
        installer->vlcversion, sysvarsGetNum (SVL_OSBITS));
    snprintf (url, sizeof (url),
        "https://get.videolan.org/vlc/last/win%zd/%s",
        sysvarsGetNum (SVL_OSBITS), installer->dlfname);
  }
  if (isMacOS ()) {
    snprintf (installer->dlfname, sizeof (installer->dlfname),
        "vlc-%s-intel64.dmg",
        installer->vlcversion);
    snprintf (url, sizeof (url),
        "https://get.videolan.org/vlc/last/macosx/%s",
        installer->dlfname);
  }
  if (*url && *installer->vlcversion) {
    webclientDownload (installer->webclient, url, installer->dlfname);
  }

  if (fileopFileExists (installer->dlfname)) {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Installing %s."), "VLC");
    installerDisplayText (installer, "-- ", tbuff);
    /* CONTEXT: installer: status message */
    installerDisplayText (installer, "   ", _("Please wait..."));
    installer->instState = INST_VLC_INSTALL;
  } else {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Download of %s failed."), "VLC");
    installerDisplayText (installer, "-- ", tbuff);
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
      snprintf (tbuff, sizeof (tbuff), "./%s", installer->dlfname);
    }
    system (tbuff);
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("%s installed."), "VLC");
    installerDisplayText (installer, "-- ", tbuff);
  }
  fileopDelete (installer->dlfname);
  installerCheckPackages (installer);
  installer->instState = INST_PYTHON_CHECK;
}

static void
installerPythonCheck (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];

  if (installer->pythoninstalled) {
    installer->instState = INST_MUTAGEN_CHECK;
    return;
  }

  if (chdir (installer->datatopdir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->datatopdir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  installerPythonGetVersion (installer);
  /* windows 7 must have an earlier versoin */
  if (strcmp (sysvarsGetStr (SV_OSVERS), "6.1") == 0) {
    strlcpy (installer->pyversion, "3.8.12", sizeof (installer->pyversion));
  }

  if (*installer->pyversion) {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Downloading %s."), "Python");
    installerDisplayText (installer, "-- ", tbuff);
    installer->instState = INST_PYTHON_DOWNLOAD;
  } else {
    snprintf (tbuff, sizeof (tbuff),
        /* CONTEXT: installer: status message */
        _("Unable to determine %s version."), "Python");
    installerDisplayText (installer, "-- ", tbuff);
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
    installerDisplayText (installer, "-- ", tbuff);
    /* CONTEXT: installer: status message */
    installerDisplayText (installer, "   ", _("Please wait..."));
    installer->instState = INST_PYTHON_INSTALL;
  } else {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Download of %s failed."), "Python");
    installerDisplayText (installer, "-- ", tbuff);
    installer->instState = INST_MUTAGEN_CHECK;
  }
}

static void
installerPythonInstall (installer_t *installer)
{
  char    tbuff [MAXPATHLEN];

  if (fileopFileExists (installer->dlfname)) {
    if (isWindows ()) {
      snprintf (tbuff, sizeof (tbuff), ".\\%s", installer->dlfname);
    }
    system (tbuff);
    snprintf (tbuff, sizeof (tbuff), _("%s installed."), "Python");
    installerDisplayText (installer, "-- ", tbuff);
  }
  fileopDelete (installer->dlfname);
  installerCheckPackages (installer);
  installer->instState = INST_MUTAGEN_CHECK;
}

static void
installerMutagenCheck (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];

  if (! installer->pythoninstalled) {
    if (isWindows ()) {
      installer->instState = INST_UPDATE_CA_FILE_INIT;
    } else {
      installer->instState = INST_UPDATE_PROCESS_INIT;
    }
    return;
  }

  if (chdir (installer->datatopdir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->datatopdir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  /* CONTEXT: installer: status message */
  snprintf (tbuff, sizeof (tbuff), _("Installing %s."), "Mutagen");
  installerDisplayText (installer, "-- ", tbuff);
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "   ", _("Please wait..."));
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
  system (tbuff);
  snprintf (tbuff, sizeof (tbuff), _("%s installed."), "Mutagen");
  installerDisplayText (installer, "-- ", tbuff);
  installerCheckPackages (installer);
  if (isWindows ()) {
    installer->instState = INST_UPDATE_CA_FILE_INIT;
  } else {
    installer->instState = INST_UPDATE_PROCESS_INIT;
  }
}

static void
installerUpdateCAFileInit (installer_t *installer)
{
  char    tbuff [200];

  if (chdir (installer->datatopdir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->datatopdir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  /* CONTEXT: installer: status message */
  snprintf (tbuff, sizeof (tbuff), _("Updating certificates."));
  installerDisplayText (installer, "-- ", tbuff);
  installer->instState = INST_UPDATE_CA_FILE;
}

static void
installerUpdateCAFile (installer_t *installer)
{
  if (installer->webclient == NULL) {
    installer->webclient = webclientAlloc (installer, installerWebResponseCallback);
  }
  webclientDownload (installer->webclient,
      "https://curl.se/ca/cacert.pem", "http/curl-ca-bundle.crt");

  installer->instState = INST_UPDATE_PROCESS_INIT;
}

static void
installerUpdateProcessInit (installer_t *installer)
{
  char  buff [MAXPATHLEN];

  /* CONTEXT: installer: status message */
  snprintf (buff, sizeof (buff), _("Updating %s."), BDJ4_LONG_NAME);
  installerDisplayText (installer, "-- ", buff);
  installer->instState = INST_UPDATE_PROCESS;
}

static void
installerUpdateProcess (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];
  char  *targv [10];
  int   targc = 0;

  snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4%s",
      installer->rundir, sysvarsGetStr (SV_OS_EXEC_EXT));
  targv [targc++] = tbuff;
  targv [targc++] = "--bdj4updater";
  /* only need to run the 'newinstall' update process when the template */
  /* files have been copied */
  if (installer->newinstall || installer->reinstall) {
    targv [targc++] = "--newinstall";
  }
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);

  installer->instState = INST_REGISTER_INIT;
}

static void
installerRegisterInit (installer_t *installer)
{
  char    tbuff [200];

  if (chdir (installer->datatopdir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->datatopdir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  /* create the new install flag file on a new install */
  if (installer->newinstall) {
    FILE  *fh;

    fh = fopen (INST_NEW_FILE, "w");
    fclose (fh);
  }

  if (strcmp (sysvarsGetStr (SV_USER), "bll") == 0 &&
      strcmp (sysvarsGetStr (SV_BDJ4_RELEASELEVEL), "") != 0) {
    /* no need to translate */
    snprintf (tbuff, sizeof (tbuff), "Registration Skipped.");
    installerDisplayText (installer, "-- ", tbuff);
    installer->instState = INST_FINISH;
  } else {
    /* CONTEXT: installer: status message */
    snprintf (tbuff, sizeof (tbuff), _("Registering %s."), BDJ4_NAME);
    installerDisplayText (installer, "-- ", tbuff);
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
      "&new=%d&overwrite=%d&update=%d&convert=%d",
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
  char  *targv [10];

  if (installer->freetarget && installer->target != NULL) {
    free (installer->target);
  }
  if (installer->freebdj3loc && installer->bdj3loc != NULL) {
    free (installer->bdj3loc);
  }
  if (installer->convlist != NULL) {
    slistFree (installer->convlist);
  }
  if (installer->tclshloc != NULL) {
    free (installer->tclshloc);
  }

  if (installer->webclient != NULL) {
    webclientClose (installer->webclient);
  }

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
    system (buff);
  }
}

static void
installerDisplayText (installer_t *installer, char *pfx, char *txt)
{
  if (installer->guienabled) {
    uiTextBoxAppendStr (installer->disptb, pfx);
    uiTextBoxAppendStr (installer->disptb, txt);
    uiTextBoxAppendStr (installer->disptb, "\n");
    installer->scrolltoend = true;
  } else {
    printf ("%s%s\n", pfx, txt);
    fflush (stdout);
  }
}

static void
installerGetTargetSaveFname (installer_t *installer, char *buff, size_t sz)
{
  if (isWindows ()) {
    snprintf (buff, sz, "%s/AppData/Roaming/BDJ4/%s",
        installer->home, INST_SAVE_FNAME);
  } else {
    snprintf (buff, sz, "%s/.config/BDJ4/%s",
        installer->home, INST_SAVE_FNAME);
  }
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
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "- copy file: %s", from);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "         to: %s", to);
  filemanipBackup (to, 1);
  filemanipCopy (from, to);
}

static void
installerSetrundir (installer_t *installer, const char *dir)
{
  installer->rundir [0] = '\0';
  if (*dir) {
    strlcpy (installer->rundir, dir, MAXPATHLEN);
    if (isMacOS ()) {
      strlcat (installer->rundir, "/Contents/MacOS", MAXPATHLEN);
    }
  }
}

static void
installerVLCGetVersion (installer_t *installer)
{
  char      *p;
  char      *e;

  *installer->vlcversion = '\0';
  installer->webresponse = NULL;
  if (installer->webclient == NULL) {
    installer->webclient = webclientAlloc (installer, installerWebResponseCallback);
  }
  webclientGet (installer->webclient, "http://get.videolan.org/vlc/last/macosx/");

  if (installer->webresponse != NULL) {
    char *srchvlc = "vlc-";
    /* vlc-3.0.16-intel64.dmg */
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

  sysvarsCheckPaths ();

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
      snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "Python");
      uiLabelSetText (&installer->pythonMsg, tbuff);
    }
    installer->pythoninstalled = true;
  } else {
    if (installer->uiBuilt) {
      snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Python");
      uiLabelSetText (&installer->pythonMsg, tbuff);
      snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Mutagen");
      uiLabelSetText (&installer->mutagenMsg, tbuff);
    }
    installer->pythoninstalled = false;
  }

  if (installer->pythoninstalled) {
    tmp = sysvarsGetStr (SV_PYTHON_MUTAGEN);
    if (installer->uiBuilt) {
      if (*tmp) {
        snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "Mutagen");
        uiLabelSetText (&installer->mutagenMsg, tbuff);
      } else {
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
