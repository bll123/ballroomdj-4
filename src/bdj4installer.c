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

#include <gtk/gtk.h>

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
#include "uiutils.h"
#include "webclient.h"

/* installation states */
typedef enum {
  INST_BEGIN,
  INST_DELAY,
  INST_INIT,
  INST_SAVE,
  INST_MAKE_TARGET,
  INST_COPY_START,
  INST_COPY_FILES,
  INST_CHDIR,
  INST_CREATE_DIRS,
  INST_CLEAN,
  INST_COPY_TEMPLATES,
  INST_CONVERT_START,
  INST_CONVERT,
  INST_CONVERT_WAIT,
  INST_CONVERT_FINISH,
  INST_CREATE_SHORTCUT,
  INST_UPDATE_PROCESS,
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
  INST_REGISTER_INIT,
  INST_REGISTER,
  INST_FINISH,
  INST_EXIT,
} installstate_t;

typedef struct {
  installstate_t  instState;
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
  int             delayMax;
  int             delayCount;
  installstate_t  delayState;
  webclient_t     *webclient;
  char            *webresponse;
  size_t          webresplen;
  /* conversion */
  char            *bdj3loc;
  char            *tclshloc;
  slist_t         *convlist;
  slistidx_t      convidx;
  uientry_t  targetEntry;
  uientry_t  bdj3locEntry;
  /* gtk */
  GtkWidget       *window;
  GtkWidget       *reinstWidget;
  GtkWidget       *feedbackMsg;
  GtkWidget       *convWidget;
  GtkWidget       *convFeedbackMsg;
  GtkWidget       *vlcMsg;
  GtkWidget       *pythonMsg;
  GtkWidget       *mutagenMsg;
  GtkTextBuffer   *dispBuffer;
  GtkWidget       *dispTextView;
  mstime_t        validateTimer;
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
} installer_t;

#define INST_TEMP_FILE  "tmp/bdj4instout.txt"
#define INST_SAVE_FNAME "installdir.txt"
#define CONV_TEMP_FILE "tmp/bdj4convout.txt"
#define BDJ3_LOC_FILE "install/bdj3loc.txt"

static void installerBuildUI (installer_t *installer);
static int  installerMainLoop (void *udata);
static void installerExit (GtkButton *b, gpointer udata);
static void installerCheckDir (GtkButton *b, gpointer udata);
static void installerSelectDirDialog (GtkButton *b, gpointer udata);
static void installerValidateDir (installer_t *installer);
static void installerValidateStart (GtkEditable *e, gpointer udata);
static void installerCheckConvert (GtkButton *b, gpointer udata);
static void installerSetConvert (installer_t *installer, int val);
static void installerDisplayConvert (installer_t *installer);
static void installerInstall (GtkButton *b, gpointer udata);
static bool installerCheckTarget (installer_t *installer, const char *dir);
static void installerSetPaths (installer_t *installer);

static void installerDelay (installer_t *installer);
static void installerInstInit (installer_t *installer);
static void installerSaveTargetDir (installer_t *installer);
static void installerMakeTarget (installer_t *installer);
static void installerCopyStart (installer_t *installer);
static void installerCopyFiles (installer_t *installer);
static void installerChangeDir (installer_t *installer);
static void installerCreateDirs (installer_t *installer);
static void installerCleanOldFiles (installer_t *installer);
static void installerCopyTemplates (installer_t *installer);
static void installerConvertStart (installer_t *installer);
static void installerConvert (installer_t *installer);
static void installerConvertFinish (installer_t *installer);
static void installerCreateShortcut (installer_t *installer);
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
static void installerScrollToEnd (GtkWidget *w, GtkAllocation *retAllocSize, gpointer udata);;
static void installerGetTargetSaveFname (installer_t *installer, char *buff, size_t len);
static void installerGetBDJ3Fname (installer_t *installer, char *buff, size_t len);
static void installerTemplateCopy (char *from, char *to);
static void installerSetrundir (installer_t *installer, const char *dir);
static void installerVLCGetVersion (installer_t *installer);
static void installerPythonGetVersion (installer_t *installer);
static void installerCleanFiles (char *fname);
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
  installer.instState = INST_BEGIN;
  installer.target = buff;
  installer.rundir [0] = '\0';
  installer.locale [0] = '\0';
  installer.bdj3loc = bdj3buff;
  installer.convidx = 0;
  installer.convlist = NULL;
  installer.tclshloc = NULL;
  installer.window = NULL;
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
  installer.delayMax = 10;
  installer.delayCount = 0;
  installer.window = NULL;
  installer.reinstWidget = NULL;
  installer.feedbackMsg = NULL;
  installer.convWidget = NULL;
  installer.convFeedbackMsg = NULL;
  installer.vlcMsg = NULL;
  installer.pythonMsg = NULL;
  installer.mutagenMsg = NULL;
  installer.dispBuffer = NULL;
  installer.dispTextView = NULL;
  getcwd (installer.currdir, sizeof (installer.currdir));
  mstimeset (&installer.validateTimer, 3600000);
  installer.webclient = NULL;
  strcpy (installer.vlcversion, "");
  strcpy (installer.pyversion, "");

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

  /* want to do an initial validation */
  if (installer.guienabled) {
    mstimeset (&installer.validateTimer, 500);
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
    uiUIInitialize ();
    if (isWindows () || isMacOS ()) {
      char *uifont;

      uifont = "Arial 12";
      uiSetUIFont (uifont);
    }
  }

  installerCheckPackages (&installer);

  if (installer.guienabled) {
    installerBuildUI (&installer);
  } else {
    installer.delayMax = 5;
    installer.instState = INST_INIT;
  }

  while (installer.instState != INST_EXIT) {
    installerMainLoop (&installer);
    mssleep (50);
  }

  installerCleanup (&installer);
  logEnd ();
  return 0;
}

static void
installerBuildUI (installer_t *installer)
{
  GtkWidget     *window;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *widget;
  GtkWidget     *scwidget;
  GtkWidget     *image;
  UIWidget      sg;
  char          tbuff [100];
  char          imgbuff [MAXPATHLEN];



  strlcpy (imgbuff, "img/bdj4_icon_inst.svg", sizeof (imgbuff));
  /* CONTEXT: installer: window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Installer"), BDJ4_NAME);
// ### close window handler
  window = uiCreateMainWindow (tbuff, imgbuff, NULL, installer);
  uiWindowSetDefaultSize (window, 1000, 600);
  installer->window = window;

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 10);
  uiWidgetSetMarginTop (vbox, 20);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiBoxPackInWindow (window, vbox);

  widget = uiCreateLabel (
      /* CONTEXT: installer */
      _("Enter the destination folder where BDJ4 will be installed."));
  uiBoxPackStart (vbox, widget);

  widget = uiEntryCreate (&installer->targetEntry);
  uiEntrySetValue (&installer->targetEntry, installer->target);
  uiWidgetAlignHorizFill (widget);
  uiWidgetExpandHoriz (widget);
  uiBoxPackStart (vbox, widget);

  g_signal_connect (installer->targetEntry.entry, "changed",
      G_CALLBACK (installerValidateStart), installer);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: installer: overwrite the previous BDJ4 installation */
  installer->reinstWidget = gtk_check_button_new_with_label (_("Overwrite"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (installer->reinstWidget),
      installer->reinstall);
  uiBoxPackStart (hbox, installer->reinstWidget);
  g_signal_connect (installer->reinstWidget, "toggled",
      G_CALLBACK (installerCheckDir), installer);

  installer->feedbackMsg = uiCreateLabel ("");
  uiSetCss (installer->feedbackMsg,
      "label { color: #ffa600; }");
  uiBoxPackStart (hbox, installer->feedbackMsg);

  widget = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  uiWidgetSetMarginTop (widget, uiBaseMarginSz);
  uiSetCss (widget,
      "separator { min-height: 4px; background-color: #733000; }");
  uiBoxPackStart (vbox, widget);

  /* conversion process */
  snprintf (tbuff, sizeof (tbuff),
      /* CONTEXT: installer */
      _("Enter the folder where %s is installed."), BDJ3_NAME);
  widget = uiCreateLabel (tbuff);
  uiBoxPackStart (vbox, widget);

  widget = uiCreateLabel (
      /* CONTEXT: installer */
      _("If there is no BallroomDJ 3 installation, leave the entry blank."));
  uiBoxPackStart (vbox, widget);

  widget = uiCreateLabel (
      /* CONTEXT: installer */
      _("The conversion process will only run for new installations and for re-installations."));
  uiBoxPackStart (vbox, widget);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: installer: label for entry field asking for BDJ3 location */
  snprintf (tbuff, sizeof (tbuff), _("%s Location"), BDJ3_NAME);
  widget = uiCreateColonLabel (tbuff);
  uiBoxPackStart (hbox, widget);

  widget = uiEntryCreate (&installer->bdj3locEntry);
  uiEntrySetValue (&installer->bdj3locEntry, installer->bdj3loc);
  uiWidgetAlignHorizFill (widget);
  uiWidgetExpandHoriz (widget);
  uiBoxPackStart (hbox, widget);
  g_signal_connect (installer->bdj3locEntry.entry, "changed",
      G_CALLBACK (installerValidateStart), installer);

  widget = uiCreateButton (NULL, "", NULL,
      installerSelectDirDialog, installer);
  image = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  uiWidgetSetMarginStart (widget, 0);
  uiBoxPackStart (hbox, widget);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: installer: convert the BallroomDJ 3 installation */
  snprintf (tbuff, sizeof (tbuff), _("Convert %s"), BDJ3_NAME);
  installer->convWidget = gtk_check_button_new_with_label (tbuff);
  uiBoxPackStart (hbox, installer->convWidget);
  g_signal_connect (installer->convWidget, "toggled",
      G_CALLBACK (installerCheckConvert), installer);

  installer->convFeedbackMsg = uiCreateLabel ("");
  uiSetCss (installer->convFeedbackMsg,
      "label { color: #ffa600; }");
  uiBoxPackStart (hbox, installer->convFeedbackMsg);

  /* VLC status */

  widget = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  uiWidgetSetMarginTop (widget, uiBaseMarginSz);
  uiSetCss (widget,
      "separator { min-height: 4px; background-color: #733000; }");
  uiBoxPackStart (vbox, widget);

  uiCreateSizeGroupHoriz (&sg);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  widget = uiCreateColonLabel ("VLC");
  uiBoxPackStart (hbox, widget);
  uiSizeGroupAdd (&sg, widget);

  installer->vlcMsg = uiCreateLabel ("");
  uiSetCss (installer->vlcMsg,
      "label { color: #ffa600; }");
  uiBoxPackStart (hbox, installer->vlcMsg);

  /* python status */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  widget = uiCreateColonLabel ("Python");
  uiBoxPackStart (hbox, widget);
  uiSizeGroupAdd (&sg, widget);

  installer->pythonMsg = uiCreateLabel ("");
  uiSetCss (installer->pythonMsg,
      "label { color: #ffa600; }");
  uiBoxPackStart (hbox, installer->pythonMsg);

  /* mutagen status */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  widget = uiCreateColonLabel ("Mutagen");
  uiBoxPackStart (hbox, widget);
  uiSizeGroupAdd (&sg, widget);

  installer->mutagenMsg = uiCreateLabel ("");
  uiSetCss (installer->mutagenMsg,
      "label { color: #ffa600; }");
  uiBoxPackStart (hbox, installer->mutagenMsg);

  /* button box */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  widget = uiCreateButton (NULL, _("Exit"), NULL,
      installerExit, installer);
  uiBoxPackEnd (hbox, widget);

  /* CONTEXT: installer: start the installation process */
  widget = uiCreateButton (NULL, _("Install"), NULL,
      installerInstall, installer);
  uiBoxPackEnd (hbox, widget);

  scwidget = uiCreateScrolledWindow (200);
  uiBoxPackStartExpand (vbox, scwidget);

  installer->dispBuffer = gtk_text_buffer_new (NULL);
  installer->dispTextView = gtk_text_view_new_with_buffer (installer->dispBuffer);
  gtk_widget_set_size_request (installer->dispTextView, -1, 400);
  uiWidgetDisableFocus (installer->dispTextView);
  uiWidgetAlignHorizFill (installer->dispTextView);
  uiWidgetAlignVertStart (installer->dispTextView);
  uiWidgetExpandHoriz (installer->dispTextView);
  uiWidgetExpandVert (installer->dispTextView);
  uiBoxPackInWindow (scwidget, installer->dispTextView);
  g_signal_connect (installer->dispTextView,
      "size-allocate", G_CALLBACK (installerScrollToEnd), installer);

  /* push the text view to the top */
  hbox = uiCreateHorizBox ();
  assert (hbox != NULL);
  uiWidgetExpandVert (hbox);
  uiBoxPackStart (vbox, hbox);

  uiWidgetShowAll (window);

  installerDisplayConvert (installer);
  installerCheckPackages (installer);
}

static int
installerMainLoop (void *udata)
{
  installer_t *installer = udata;

  if (installer->guienabled) {
    uiUIProcessEvents ();
  }

  if (mstimeCheck (&installer->validateTimer)) {
    installerValidateDir (installer);
    mstimeset (&installer->validateTimer, 3600000);
  }

  switch (installer->instState) {
    case INST_BEGIN: {
      /* do nothing */
      break;
    }
    case INST_DELAY: {
      installerDelay (installer);
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
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "target: %s", installer->target);

      break;
    }
    case INST_CLEAN: {
      installerCleanOldFiles (installer);
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
    case INST_UPDATE_PROCESS: {
      installerUpdateProcess (installer);
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
    case INST_REGISTER_INIT: {
      installerRegisterInit (installer);
      break;
    }
    case INST_REGISTER: {
      installerRegister (installer);
      break;
    }
    case INST_FINISH: {
      /* CONTEXT: installer */
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

static void
installerCheckDir (GtkButton *b, gpointer udata)
{
  installer_t   *installer = udata;

  installerValidateDir (installer);
}

static void
installerValidateDir (installer_t *installer)
{
  bool          locok = false;
  const char    *dir;
  bool          exists = false;
  const char    *fn;
  char          tbuff [100];

  if (! installer->guienabled) {
    return;
  }

  if (installer->feedbackMsg == NULL ||
      installer->convFeedbackMsg == NULL ||
      installer->convWidget == NULL ||
      installer->reinstWidget == NULL ||
      installer->vlcMsg == NULL ||
      installer->pythonMsg == NULL ||
      installer->mutagenMsg == NULL) {
    mstimeset (&installer->validateTimer, 500);
    return;
  }

  dir = uiEntryGetValue (&installer->targetEntry);
  installer->reinstall = gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (installer->reinstWidget));
  uiLabelSetText (installer->feedbackMsg, "");

  exists = fileopIsDirectory (dir);
  if (exists) {
    exists = installerCheckTarget (installer, dir);
    if (exists) {
      if (installer->reinstall) {
        /* CONTEXT: installer: message indicating the action that will be taken */
        snprintf (tbuff, sizeof (tbuff), _("Overwriting existing %s installation."), BDJ4_NAME);
        uiLabelSetText (installer->feedbackMsg, tbuff);
        installerSetConvert (installer, TRUE);
      } else {
        /* CONTEXT: installer: message indicating the action that will be taken */
        snprintf (tbuff, sizeof (tbuff), _("Updating existing %s installation."), BDJ4_NAME);
        uiLabelSetText (installer->feedbackMsg, tbuff);
        installerSetConvert (installer, FALSE);
      }
    } else {
      /* CONTEXT: installer: the selected folder exists and is not a BDJ4 installation */
      uiLabelSetText (installer->feedbackMsg, _("Error: Folder already exists."));
      installerSetConvert (installer, FALSE);
    }
  } else {
    /* CONTEXT: installer: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("New %s installation."), BDJ4_NAME);
    uiLabelSetText (installer->feedbackMsg, tbuff);
    installerSetConvert (installer, TRUE);
  }

  /* bdj3 location validation */

  if (installer->bdj3locEntry.buffer == NULL) {
    mstimeset (&installer->validateTimer, 500);
    return;
  }

  fn = uiEntryGetValue (&installer->bdj3locEntry);
  if (*fn == '\0' || strcmp (fn, "-") == 0 || locationcheck (fn)) {
    locok = true;
  }

  if (! locok) {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (installer->bdj3locEntry.entry),
        GTK_ENTRY_ICON_SECONDARY, "dialog-error");
    installerSetConvert (installer, FALSE);
  } else {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (installer->bdj3locEntry.entry),
        GTK_ENTRY_ICON_SECONDARY, NULL);
  }

  installerSetPaths (installer);
  installerDisplayConvert (installer);
}

static void
installerValidateStart (GtkEditable *e, gpointer udata)
{
  installer_t   *installer = udata;

  if (! installer->guienabled) {
    return;
  }

  /* if the user is typing, clear the message */
  uiLabelSetText (installer->feedbackMsg, "");
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (installer->bdj3locEntry.entry),
      GTK_ENTRY_ICON_SECONDARY, NULL);
  mstimeset (&installer->validateTimer, 500);
}

static void
installerSelectDirDialog (GtkButton *b, gpointer udata)
{
  installer_t     *installer = udata;
  char            *fn = NULL;
  uiselect_t selectdata;
  char            tbuff [100];

  /* CONTEXT: installer: label for entry field for BDJ3 location */
  snprintf (tbuff, sizeof (tbuff), _("Select %s Location"), BDJ3_NAME);
  selectdata.label = tbuff;
  selectdata.window = installer->window;
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
}

static void
installerCheckConvert (GtkButton *b, gpointer udata)
{
  installer_t   *installer = udata;

  if (installer->inSetConvert) {
    return;
  }

  installerDisplayConvert (installer);
}

static void
installerSetConvert (installer_t *installer, int val)
{
  if (installer->convWidget == NULL ||
     installer->convFeedbackMsg == NULL) {
    return;
  }

  gtk_toggle_button_set_active (
      GTK_TOGGLE_BUTTON (installer->convWidget), val);
}

static void
installerDisplayConvert (installer_t *installer)
{
  int           nval;
  char          *tptr;

  if (installer->convWidget == NULL ||
     installer->convFeedbackMsg == NULL) {
    return;
  }

  nval = gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (installer->convWidget));

  installer->inSetConvert = true;

  if (strcmp (installer->bdj3loc, "-") == 0 ||
      *installer->bdj3loc == '\0') {
    nval = 0;
    gtk_toggle_button_set_active (
        GTK_TOGGLE_BUTTON (installer->convWidget), nval);
  }

  installer->inSetConvert = false;

  installer->convprocess = nval;

  if (nval) {
    /* CONTEXT: installer: message indicating the conversion action that will be taken */
    tptr = _("Conversion will be processed");
    uiLabelSetText (installer->convFeedbackMsg, tptr);
  } else {
    /* CONTEXT: installer: message indicating the conversion action that will be taken */
    tptr = _("No conversion.");
    uiLabelSetText (installer->convFeedbackMsg, tptr);
  }
}

static void
installerExit (GtkButton *b, gpointer udata)
{
  installer_t   *installer = udata;

  installer->instState = INST_EXIT;
  return;
}

static void
installerInstall (GtkButton *b, gpointer udata)
{
  installer_t *installer = udata;

  if (installer->instState == INST_BEGIN) {
    installer->instState = INST_INIT;
  }
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
installerDelay (installer_t *installer)
{
  installer->instState = INST_DELAY;
  ++installer->delayCount;
  if (installer->delayCount > installer->delayMax) {
    installer->instState = installer->delayState;
  }
}

static void
installerInstInit (installer_t *installer)
{
  bool        exists = false;
  char        tbuff [MAXPATHLEN];

  if (installer->guienabled) {
    installerSetPaths (installer);
  }

  if (installer->guienabled) {
    installer->reinstall = gtk_toggle_button_get_active (
        GTK_TOGGLE_BUTTON (installer->reinstWidget));
  }

  if (! installer->guienabled) {
    tbuff [0] = '\0';
    /* CONTEXT: installer: command line interface: asking for the BDJ4 destination */
    printf (_("Enter the destination folder."));
    printf ("\n");
    /* CONTEXT: installer: command line interface: */
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
          /* CONTEXT: installer: command line interface: */
          printf (" * %s", _("Installation aborted."));
          printf ("\n");
          fflush (stdout);
          installer->instState = INST_BEGIN;
          return;
        }
      }
    }

    if (! exists) {
      /* CONTEXT: installer: command line interface: */
      printf (_("New %s installation."), BDJ4_NAME);

      /* do not allow an overwrite of an existing directory that is not bdj4 */
      if (installer->guienabled) {
        uiLabelSetText (installer->feedbackMsg, _("Folder already exists."));
      }

      /* CONTEXT: installer: command line interface: the selected folder exists and is not a BDJ4 installation */
      snprintf (tbuff, sizeof (tbuff), _("Error: Folder %s already exists."),
          installer->target);
      installerDisplayText (installer, "", tbuff);
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
  fileopMakeDir (installer->target);
  fileopMakeDir (installer->rundir);

  installer->instState = INST_COPY_START;
}

static void
installerCopyStart (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Copying files."));
  installerDisplayText (installer, "   ", _("Please wait..."));

  /* the unpackdir is not necessarily the same as the current dir */
  /* on mac os, they are different */
  if (chdir (installer->unpackdir) < 0) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->unpackdir);
    /* CONTEXT: installer: failure message */
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
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

  installer->instState = INST_CLEAN;
}

static void
installerCleanOldFiles (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Cleaning old files."));

  if (chdir (installer->rundir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->rundir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  installerCleanFiles ("install/cleanuplist.txt");
  installer->instState = INST_COPY_TEMPLATES;
}


static void
installerCopyTemplates (installer_t *installer)
{
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

  snprintf (tbuff, sizeof (tbuff), "%s/install/%s", installer->rundir,
      "localized-sr.txt");
  srdf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);
  snprintf (tbuff, sizeof (tbuff), "%s/install/%s", installer->rundir,
      "localized-auto.txt");
  autodf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);
  snprintf (tbuff, sizeof (tbuff), "%s/install/%s", installer->rundir,
      "localized-qd.txt");
  qddf = datafileAllocParse ("loc-qd", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);

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
    if (strcmp (fname, "volintfc.txt") == 0) {
      continue;
    }
    if (strcmp (fname, "playerintfc.txt") == 0) {
      continue;
    }

    if (strcmp (fname, "bdj-flex-dark.html") == 0) {
      snprintf (from, sizeof (from), "%s/templates/%s",
          installer->rundir, fname);
      snprintf (to, sizeof (to), "http/bdj4remote.html");
      installerTemplateCopy (from, to);
      continue;
    }
    if (strcmp (fname, "mobilemq.html") == 0) {
      snprintf (from, sizeof (from), "%s/templates/%s",
          installer->rundir, fname);
      snprintf (to, sizeof (to), "http/%s", fname);
      installerTemplateCopy (from, to);
      continue;
    }

    pi = pathInfo (fname);
    if (pathInfoExtCheck (pi, ".html")) {
      free (pi);
      continue;
    }

    if (pathInfoExtCheck (pi, ".crt")) {
      snprintf (from, sizeof (from), "%s/templates/%s",
          installer->rundir, fname);
      snprintf (to, sizeof (to), "http/%s", fname);
    } else if (pathInfoExtCheck (pi, ".svg")) {
      snprintf (from, sizeof (from), "%s/templates/%s",
          installer->rundir, fname);
      snprintf (to, sizeof (to), "%s/img/%s",
          installer->rundir, fname);
    } else if (strncmp (fname, "bdjconfig", 9) == 0) {
      snprintf (from, sizeof (from), "%s/templates/%s",
          installer->rundir, fname);

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

      snprintf (from, sizeof (from), "%s/templates/%s",
          installer->rundir, tbuff);
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

    installerTemplateCopy (from, to);

    free (pi);
  }
  slistFree (dirlist);

  snprintf (from, sizeof (from), "%s/img/favicon.ico", installer->rundir);
  snprintf (to, sizeof (to), "http/favicon.ico");
  installerTemplateCopy (from, to);

  snprintf (from, sizeof (from), "%s/img/led_on.svg", installer->rundir);
  snprintf (to, sizeof (to), "http/led_on.svg");
  installerTemplateCopy (from, to);

  snprintf (from, sizeof (from), "%s/img/led_off.svg", installer->rundir);
  snprintf (to, sizeof (to), "http/led_off.svg");
  installerTemplateCopy (from, to);

  snprintf (from, sizeof (from), "%s/img/ballroomdj4.svg", installer->rundir);
  snprintf (to, sizeof (to), "http/ballroomdj4.svg");
  installerTemplateCopy (from, to);

  snprintf (from, sizeof (from), "%s/img/mrc", installer->rundir);
  snprintf (to, sizeof (to), "http/mrc");
  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl \"%s\" \"%s\"",
        from, to);
    system (tbuff);
  } else {
    snprintf (tbuff, sizeof (tbuff), "cp -r '%s' '%s'", from, "http");
  }

  datafileFree (srdf);
  datafileFree (autodf);

  installer->instState = INST_CONVERT_START;
}

/* conversion routines */

static void
installerConvertStart (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];
  char  *locs [15];
  int   locidx = 0;

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
    /* CONTEXT: installer: command line interface */
    printf (_("Enter the folder where %s is installed."), BDJ3_NAME);
    printf ("\n");
    /* CONTEXT: installer: command line interface */
    printf (_("The conversion process will only run for new installations and for re-installs."));
    printf ("\n");
    /* CONTEXT: installer: command line interface: accept BDJ3 location default */
    printf (_("Press 'Enter' to select the default."));
    printf ("\n");
    /* CONTEXT: installer: command line interface */
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

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Starting conversion process."));

  installer->convlist = filemanipBasicDirList ("conv", ".tcl");
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
    installer->delayCount = 0;
    installer->delayState = INST_VLC_DOWNLOAD;
    installer->instState = INST_DELAY;
  } else {
    snprintf (tbuff, sizeof (tbuff),
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
    installerDisplayText (installer, "   ", _("Please wait..."));
    installer->delayCount = 0;
    installer->delayState = INST_VLC_INSTALL;
    installer->instState = INST_DELAY;
  } else {
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
    installerValidateDir (installer);
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
    installer->delayCount = 0;
    installer->delayState = INST_PYTHON_DOWNLOAD;
    installer->instState = INST_DELAY;
  } else {
    snprintf (tbuff, sizeof (tbuff),
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
    installerDisplayText (installer, "   ", _("Please wait..."));
    installer->delayCount = 0;
    installer->delayState = INST_PYTHON_INSTALL;
    installer->instState = INST_DELAY;
  } else {
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
    installerValidateDir (installer);
    /* CONTEXT: installer: status message */
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
      installer->instState = INST_REGISTER_INIT;
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
  installerDisplayText (installer, "   ", _("Please wait..."));
  installer->delayCount = 0;
  installer->delayState = INST_MUTAGEN_INSTALL;
  installer->instState = INST_DELAY;
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
  /* CONTEXT: installer: status message */
  snprintf (tbuff, sizeof (tbuff), _("%s installed."), "Mutagen");
  installerDisplayText (installer, "-- ", tbuff);
  installerCheckPackages (installer);
  if (isWindows ()) {
    installer->instState = INST_UPDATE_CA_FILE_INIT;
  } else {
    installer->instState = INST_REGISTER_INIT;
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
  installer->delayCount = 0;
  installer->delayState = INST_UPDATE_CA_FILE;
  installer->instState = INST_DELAY;
}

static void
installerUpdateCAFile (installer_t *installer)
{
  if (installer->webclient == NULL) {
    installer->webclient = webclientAlloc (installer, installerWebResponseCallback);
  }
  webclientDownload (installer->webclient,
      "https://curl.se/ca/cacert.pem", "http/curl-ca-bundle.crt");

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
    installer->delayCount = 0;
    installer->delayState = INST_REGISTER;
    installer->instState = INST_DELAY;
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
      sysvarsGetStr (SV_SUPPORTMSG_HOST), sysvarsGetStr (SV_REGISTER_URI));
  snprintf (tbuff, sizeof (tbuff),
      "key=%s&version=%s&build=%s&builddate=%s&releaselevel=%s"
      "&osname=%s&osdisp=%s&osvers=%s&osbuild=%s&pythonvers=%s"
      "&user=%s&host=%s&new=%d&overwrite=%d&update=%d&convert=%d",
      "9873453",
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
  GtkTextIter iter;

  if (installer->guienabled) {
    gtk_text_buffer_get_end_iter (installer->dispBuffer, &iter);
    gtk_text_buffer_insert (installer->dispBuffer, &iter, pfx, -1);
    gtk_text_buffer_get_end_iter (installer->dispBuffer, &iter);
    gtk_text_buffer_insert (installer->dispBuffer, &iter, txt, -1);
    gtk_text_buffer_get_end_iter (installer->dispBuffer, &iter);
    gtk_text_buffer_insert (installer->dispBuffer, &iter, "\n", -1);
  } else {
    printf ("%s%s\n", pfx, txt);
    fflush (stdout);
  }
}

static void
installerScrollToEnd (GtkWidget *w, GtkAllocation *retAllocSize, gpointer udata)
{
  installer_t *installer = udata;
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter (installer->dispBuffer, &iter);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (installer->dispTextView),
      &iter, 0, false, 0, 0);
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
installerTemplateCopy (char *from, char *to)
{
  char      *localetmpldir;
  char      tbuff [MAXPATHLEN];

  localetmpldir = sysvarsGetStr (SV_LOCALE);
  strlcpy (tbuff, localetmpldir, sizeof (tbuff));
  strlcat (tbuff, "/", sizeof (tbuff));
  strlcat (tbuff, from, sizeof (tbuff));
  if (fileopFileExists (tbuff)) {
    from = tbuff;
  } else {
    localetmpldir = sysvarsGetStr (SV_LOCALE_SHORT);
    strlcpy (tbuff, localetmpldir, sizeof (tbuff));
    strlcat (tbuff, "/", sizeof (tbuff));
    strlcat (tbuff, from, MAXPATHLEN);
    if (fileopFileExists (tbuff)) {
      from = tbuff;
    }
  }
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
installerCleanFiles (char *fname)
{
  FILE    *fh;
  char    tbuff [MAXPATHLEN];

  fh = fileopOpen (fname, "r");
  if (fh != NULL) {
    while (fgets (tbuff, sizeof (tbuff), fh) != NULL) {
      stringTrim (tbuff);
      stringTrimChar (tbuff, '/');

      if (fileopIsDirectory (tbuff)) {
        filemanipDeleteDir (tbuff);
      } else {
        if (fileopFileExists (tbuff)) {
          fileopDelete (tbuff);
        }
      }
    }
    fclose (fh);
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
    /* CONTEXT: installer: display of package status */
    snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "VLC");
    if (installer->vlcMsg != NULL) {
      uiLabelSetText (installer->vlcMsg, tbuff);
    }
    installer->vlcinstalled = true;
  } else {
    /* CONTEXT: installer: display of package status */
    snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "VLC");
    if (installer->vlcMsg != NULL) {
      uiLabelSetText (installer->vlcMsg, tbuff);
    }
    installer->vlcinstalled = false;
  }

  tmp = sysvarsGetStr (SV_PATH_PYTHON);

  if (*tmp) {
    /* CONTEXT: installer: display of package status */
    snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "Python");
    if (installer->pythonMsg != NULL) {
      uiLabelSetText (installer->pythonMsg, tbuff);
    }
    installer->pythoninstalled = true;
  } else {
    /* CONTEXT: installer: display of package status */
    snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Python");
    if (installer->pythonMsg != NULL) {
      uiLabelSetText (installer->pythonMsg, tbuff);
    }
    /* CONTEXT: installer: display of package status */
    snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Mutagen");
    if (installer->mutagenMsg != NULL) {
      uiLabelSetText (installer->mutagenMsg, tbuff);
    }
    installer->pythoninstalled = false;
  }

  if (installer->pythoninstalled) {
    tmp = sysvarsGetStr (SV_PYTHON_MUTAGEN);
    if (*tmp) {
      /* CONTEXT: installer: display of package status */
      snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "Mutagen");
      if (installer->mutagenMsg != NULL) {
        uiLabelSetText (installer->mutagenMsg, tbuff);
      }
    } else {
      /* CONTEXT: installer: display of package status */
      snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Mutagen");
      if (installer->mutagenMsg != NULL) {
        uiLabelSetText (installer->mutagenMsg, tbuff);
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

