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
  INST_VLC_CHECK,
  INST_VLC_DOWNLOAD,
  INST_VLC_INSTALL,
  INST_PYTHON_CHECK,
  INST_PYTHON_DOWNLOAD,
  INST_PYTHON_INSTALL,
  INST_MUTAGEN_CHECK,
  INST_MUTAGEN_INSTALL,
  INST_FINISH,
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
  int             delayCount;
  installstate_t  delayState;
  /* conversion */
  char            *bdj3loc;
  char            *tclshloc;
  slist_t         *convlist;
  slistidx_t      convidx;
  /* gtk */
  GtkApplication  *app;
  GtkWidget       *window;
  GtkWidget       *targetEntry;
  GtkWidget       *reinstWidget;
  GtkWidget       *feedbackMsg;
  GtkWidget       *vlcMsg;
  GtkWidget       *pythonMsg;
  GtkEntryBuffer  *targetBuffer;
  GtkTextBuffer   *dispBuffer;
  GtkWidget       *dispTextView;
  GtkWidget       *bdj3locEntry;        // bdj3 location
  GtkEntryBuffer  *bdj3locBuffer;
  mstime_t        validateTimer;
  /* flags */
  bool            newinstall : 1;
  bool            reinstall : 1;
  bool            freetarget : 1;
  bool            freebdj3loc : 1;
  bool            guienabled : 1;
  bool            vlcinstalled : 1;
  bool            pythoninstalled : 1;
} installer_t;

#define INST_TEMP_FILE  "tmp/bdj4instout.txt"
#define INST_SAVE_FNAME "installdir.txt"
#define CONV_TEMP_FILE "tmp/bdj4convout.txt"
#define BDJ3_LOC_FILE "install/bdj3loc.txt"

static int  installerCreateGui (installer_t *installer, int argc, char *argv []);
static void installerActivate (GApplication *app, gpointer udata);
static int  installerMainLoop (void *udata);
static void installerExit (GtkButton *b, gpointer udata);
static void installerCheckDir (GtkButton *b, gpointer udata);
static void installerSelectDirDialog (GtkButton *b, gpointer udata);
static void installerValidateDir (installer_t *installer);
static void installerValidateStart (GtkEditable *e, gpointer udata);
static void installerInstall (GtkButton *b, gpointer udata);
static bool installerCheckTarget (installer_t *installer, const char *dir);

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
static void installerVLCCheck (installer_t *installer);
static void installerVLCDownload (installer_t *installer);
static void installerVLCInstall (installer_t *installer);
static void installerPythonCheck (installer_t *installer);
static void installerPythonDownload (installer_t *installer);
static void installerPythonInstall (installer_t *installer);
static void installerMutagenCheck (installer_t *installer);
static void installerMutagenInstall (installer_t *installer);

static void installerCleanup (installer_t *installer);
static void installerDisplayText (installer_t *installer, char *pfx, char *txt);
static void installerScrollToEnd (GtkWidget *w, GtkAllocation *retAllocSize, gpointer udata);;
static void installerGetTargetFname (installer_t *installer, char *buff, size_t len);
static void installerGetBDJ3Fname (installer_t *installer, char *buff, size_t len);
static void installerTemplateCopy (char *from, char *to);
static void installerSetrundir (installer_t *installer, const char *dir);
static void installerVLCGetVersion (installer_t *installer);
static void installerPythonGetVersion (installer_t *installer);

int
main (int argc, char *argv[])
{
  installer_t   installer;
  int           status;
  char          tbuff [512];
  char          buff [MAXPATHLEN];
  char          bdj3buff [MAXPATHLEN];
  FILE          *fh;
  int           c = 0;
  int           option_index = 0;

  static struct option bdj_options [] = {
    { "installer",  no_argument,        NULL,   12 },
    { "reinstall",  no_argument,        NULL,   'r' },
    { "guidisabled",no_argument,        NULL,   'g' },
    { "unpackdir",  required_argument,  NULL,   'u' },
    { "debug",      required_argument,  NULL,   'd' },
    { "theme",      required_argument,  NULL,   't' },
    { "debugself",  no_argument,        NULL,   'D' },
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
  installer.app = NULL;
  installer.window = NULL;
  installer.targetEntry = NULL;
  installer.reinstWidget = NULL;
  installer.feedbackMsg = NULL;
  installer.targetBuffer = NULL;
  installer.dispBuffer = NULL;
  installer.dispTextView = NULL;
  installer.currdir [0] = '\0';
  installer.newinstall = true;
  installer.reinstall = false;
  installer.freetarget = false;
  installer.freebdj3loc = false;
  installer.guienabled = true;
  getcwd (installer.currdir, sizeof (installer.currdir));
  mstimeset (&installer.validateTimer, 3600000);

  while ((c = getopt_long_only (argc, argv, "p:d:t:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 12: {
        break;
      }
      case 'd': {
        break;
      }
      case 'D': {
        break;
      }
      case 't': {
        break;
      }
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

  installerGetTargetFname (&installer, tbuff, sizeof (tbuff));
  fh = fileopOpen (tbuff, "r");
  if (fh != NULL) {
    /* installer.target is pointing at buff */
    fgets (buff, sizeof (buff), fh);
    stringTrim (buff);
    fclose (fh);
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
    strlcpy (tbuff, installer.target, sizeof (tbuff));
    pathToWinPath (installer.target, tbuff, sizeof (tbuff));
  }

  if (installer.guienabled) {
    gtk_init (&argc, NULL);
    uiutilsInitGtkLog ();
    if (isWindows ()) {
      char *uifont;

      uifont = "Arial 11";
      uiutilsSetUIFont (uifont);
    }
  }

  if (installer.guienabled) {
    g_timeout_add (UI_MAIN_LOOP_TIMER * 5, installerMainLoop, &installer);
    status = installerCreateGui (&installer, 0, NULL);
  } else {
    status = 0;
    installer.instState = INST_INIT;
    while (installer.instState != INST_FINISH) {
      installerMainLoop (&installer);
      if (installer.instState == INST_BEGIN) {
        /* exit the loop */
        installer.instState = INST_FINISH;
      }
      mssleep (50);
    }
  }

  if (installer.freetarget && installer.target != NULL) {
    free (installer.target);
  }
  if (installer.freebdj3loc && installer.bdj3loc != NULL) {
    free (installer.bdj3loc);
  }
  if (installer.convlist != NULL) {
    slistFree (installer.convlist);
  }
  if (installer.tclshloc != NULL) {
    free (installer.tclshloc);
  }
  installerCleanup (&installer);
  logEnd ();
  return status;
}

static int
installerCreateGui (installer_t *installer, int argc, char *argv [])
{
  int             status;

  installer->app = gtk_application_new (
      "org.bdj4.BDJ4.installer",
      G_APPLICATION_FLAGS_NONE
  );
  g_signal_connect (installer->app, "activate", G_CALLBACK (installerActivate), installer);

  status = g_application_run (G_APPLICATION (installer->app), argc, argv);
  if (GTK_IS_WIDGET (installer->window)) {
    gtk_widget_destroy (installer->window);
  }
  g_object_unref (installer->app);
  return status;
}

static void
installerActivate (GApplication *app, gpointer udata)
{
  installer_t           *installer = udata;
  GtkWidget             *window;
  GtkWidget             *vbox;
  GtkWidget             *hbox;
  GtkWidget             *widget;
  GtkWidget             *scwidget;
  GtkWidget             *image;
  GError                *gerr = NULL;


  window = gtk_application_window_new (GTK_APPLICATION (app));

  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_window_set_focus_on_map (GTK_WINDOW (window), FALSE);
  gtk_window_set_title (GTK_WINDOW (window), _("BDJ4 Installer"));
  gtk_window_set_default_icon_from_file ("img/bdj4_icon.svg", &gerr);
  gtk_window_set_default_size (GTK_WINDOW (window), 1000, 600);
  installer->window = window;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_margin_top (vbox, 20);
  gtk_widget_set_margin_bottom (vbox, 10);
  gtk_widget_set_margin_start (vbox, 10);
  gtk_widget_set_margin_end (vbox, 10);
  gtk_widget_set_hexpand (vbox, TRUE);
  gtk_widget_set_vexpand (vbox, TRUE);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateLabel (
      _("Enter the destination folder where BDJ4 will be installed."));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  installer->targetBuffer = gtk_entry_buffer_new (installer->target, -1);
  installer->targetEntry = gtk_entry_new_with_buffer (installer->targetBuffer);
  gtk_entry_set_width_chars (GTK_ENTRY (installer->targetEntry), 80);
  gtk_entry_set_max_length (GTK_ENTRY (installer->targetEntry), MAXPATHLEN);
  gtk_entry_set_input_purpose (GTK_ENTRY (installer->targetEntry), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_halign (installer->targetEntry, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (installer->targetEntry, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), installer->targetEntry,
      TRUE, TRUE, 0);
  g_signal_connect (installer->targetEntry, "changed", G_CALLBACK (installerValidateStart), installer);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  installer->reinstWidget = gtk_check_button_new_with_label (_("Overwrite"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (installer->reinstWidget),
      installer->reinstall);
  gtk_box_pack_start (GTK_BOX (hbox), installer->reinstWidget, FALSE, FALSE, 0);
  g_signal_connect (installer->reinstWidget, "toggled",
      G_CALLBACK (installerCheckDir), installer);

  installer->feedbackMsg = uiutilsCreateLabel ("");
  uiutilsSetCss (installer->feedbackMsg,
      "label { color: #ffa600; }");
  gtk_box_pack_start (GTK_BOX (hbox), installer->feedbackMsg, FALSE, FALSE, 0);

  /* conversion process */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateLabel (
      _("Enter the folder where BallroomDJ 3 is installed."));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateLabel (
      _("The conversion process will only run for new installations and for re-installations."));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateLabel (
      _("If there is no BallroomDJ 3 installation, enter a single '-'."));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateColonLabel (_("BallroomDJ 3 Location"));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  installer->bdj3locBuffer = gtk_entry_buffer_new (installer->bdj3loc, -1);
  installer->bdj3locEntry = gtk_entry_new_with_buffer (installer->bdj3locBuffer);
  gtk_entry_set_width_chars (GTK_ENTRY (installer->bdj3locEntry), 80);
  gtk_entry_set_max_length (GTK_ENTRY (installer->bdj3locEntry), MAXPATHLEN);
  gtk_entry_set_input_purpose (GTK_ENTRY (installer->bdj3locEntry), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_halign (installer->bdj3locEntry, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (installer->bdj3locEntry, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), installer->bdj3locEntry,
      TRUE, TRUE, 0);
  g_signal_connect (installer->bdj3locEntry, "changed", G_CALLBACK (installerValidateStart), installer);

  widget = uiutilsCreateButton ("", NULL, installerSelectDirDialog, installer);
  image = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  gtk_widget_set_margin_start (widget, 0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* VLC status */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateColonLabel ("VLC");
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  installer->vlcMsg = uiutilsCreateLabel ("");
  uiutilsSetCss (installer->vlcMsg,
      "label { color: #ffa600; }");
  gtk_box_pack_start (GTK_BOX (hbox), installer->vlcMsg, FALSE, FALSE, 0);

  /* python status */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateColonLabel ("Python");
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  installer->pythonMsg = uiutilsCreateLabel ("");
  uiutilsSetCss (installer->pythonMsg,
      "label { color: #ffa600; }");
  gtk_box_pack_start (GTK_BOX (hbox), installer->pythonMsg, FALSE, FALSE, 0);

  /* button box */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Exit"), NULL, installerExit, installer);
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Install"), NULL, installerInstall, installer);
  gtk_box_pack_end (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  scwidget = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scwidget), 150);
  gtk_box_pack_start (GTK_BOX (vbox), scwidget,
      FALSE, FALSE, 0);

  installer->dispBuffer = gtk_text_buffer_new (NULL);
  installer->dispTextView = gtk_text_view_new_with_buffer (installer->dispBuffer);
  gtk_widget_set_size_request (installer->dispTextView, -1, 400);
  gtk_widget_set_can_focus (installer->dispTextView, FALSE);
  gtk_widget_set_halign (installer->dispTextView, GTK_ALIGN_FILL);
  gtk_widget_set_valign (installer->dispTextView, GTK_ALIGN_START);
  gtk_widget_set_hexpand (installer->dispTextView, TRUE);
  gtk_widget_set_vexpand (installer->dispTextView, TRUE);
  gtk_container_add (GTK_CONTAINER (scwidget), installer->dispTextView);
  g_signal_connect (installer->dispTextView,
      "size-allocate", G_CALLBACK (installerScrollToEnd), installer);

  /* push the text view to the top */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_vexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  gtk_widget_show_all (window);
}

static int
installerMainLoop (void *udata)
{
  installer_t *installer = udata;

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

      logStartAppend ("bdj4installer", "in", LOG_IMPORTANT);
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
    case INST_FINISH: {
      installerDisplayText (installer, "## ",  _("Installation complete."));
      installer->instState = INST_BEGIN;
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
  char          tbuff [MAXPATHLEN];

  if (! installer->guienabled) {
    return;
  }

  if (installer->feedbackMsg == NULL ||
      installer->targetBuffer == NULL ||
      installer->reinstWidget == NULL ||
      installer->vlcMsg == NULL ||
      installer->pythonMsg == NULL) {
    mstimeset (&installer->validateTimer, 500);
    return;
  }

  dir = gtk_entry_buffer_get_text (installer->targetBuffer);
  installer->reinstall = gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (installer->reinstWidget));
  gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), "");

  exists = fileopIsDirectory (dir);
  if (exists) {
    exists = installerCheckTarget (installer, dir);
    if (exists) {
      if (installer->reinstall) {
        gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), _("Overwriting existing BDJ4 installation."));
      } else {
        gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), _("Updating existing BDJ4 installation."));
      }
    } else {
      gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), _("Error: Folder already exists."));
    }
  } else {
    gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), _("New BDJ4 installation."));
  }

  /* bdj3 location validation */

  if (installer->bdj3locBuffer == NULL) {
    mstimeset (&installer->validateTimer, 500);
    return;
  }

  fn = gtk_entry_buffer_get_text (installer->bdj3locBuffer);
  if (*fn == '\0' || strcmp (fn, "-") == 0 || locationcheck (fn)) {
    locok = true;
  }

  if (! locok) {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (installer->bdj3locEntry),
        GTK_ENTRY_ICON_SECONDARY, "dialog-error");
  } else {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (installer->bdj3locEntry),
        GTK_ENTRY_ICON_SECONDARY, NULL);
  }

  *tbuff = '\0';
  if (isWindows ()) {
    strlcpy (tbuff, "C:/Program Files/VideoLAN/VLC", sizeof (tbuff));
  }
  if (isMacOS ()) {
    strlcpy (tbuff, "/Applications/VLC.app/Contents/MacOS/lib/", sizeof (tbuff));
  }
  if (isLinux ()) {
    strlcpy (tbuff, "/usr/lib/x86_64-linux-gnu/libvlc.so.5", sizeof (tbuff));
  }

  if (*tbuff) {
    if (fileopIsDirectory (tbuff) || fileopFileExists (tbuff)) {
      snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "VLC");
      gtk_label_set_text (GTK_LABEL (installer->vlcMsg), tbuff);
      installer->vlcinstalled = true;
    } else {
      snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "VLC");
      gtk_label_set_text (GTK_LABEL (installer->vlcMsg), tbuff);
      installer->vlcinstalled = false;
    }
  }

  *tbuff = '\0';
  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff),
        "%s/AppData/Local/Programs/Python", installer->home);
  }
  if (isMacOS ()) {
    strlcpy (tbuff, "/opt/local/bin/python3", sizeof (tbuff));
  }
  if (isLinux ()) {
    strlcpy (tbuff, "/usr/bin/python3", sizeof (tbuff));
  }

  if (*tbuff) {
    if (fileopIsDirectory (tbuff) || fileopFileExists (tbuff)) {
      snprintf (tbuff, sizeof (tbuff), _("%s is installed"), "Python");
      gtk_label_set_text (GTK_LABEL (installer->pythonMsg), tbuff);
      installer->pythoninstalled = true;
    } else {
      snprintf (tbuff, sizeof (tbuff), _("%s is not installed"), "Python");
      gtk_label_set_text (GTK_LABEL (installer->pythonMsg), tbuff);
      installer->pythoninstalled = false;
    }
  }
}

static void
installerValidateStart (GtkEditable *e, gpointer udata)
{
  installer_t   *installer = udata;

  if (! installer->guienabled) {
    return;
  }

  /* if the user is typing, clear the message */
  gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), "");
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (installer->bdj3locEntry),
      GTK_ENTRY_ICON_SECONDARY, NULL);
  mstimeset (&installer->validateTimer, 500);
}

static void
installerSelectDirDialog (GtkButton *b, gpointer udata)
{
  installer_t           *installer = udata;
  GtkFileChooserNative  *widget = NULL;
  gint                  res;
  char                  *fn = NULL;

  widget = gtk_file_chooser_native_new (
      _("Select BallroomDJ 3 Location"),
      GTK_WINDOW (installer->window),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
      _("Select"), _("Close"));

  res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (widget));
  if (res == GTK_RESPONSE_ACCEPT) {
    fn = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
    if (installer->bdj3loc != NULL && installer->freebdj3loc) {
      free (installer->bdj3loc);
    }
    installer->bdj3loc = fn;
    installer->freebdj3loc = true;
    gtk_entry_buffer_set_text (installer->bdj3locBuffer, fn, -1);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", installer->bdj3loc);
  }

  g_object_unref (widget);
}

static void
installerExit (GtkButton *b, gpointer udata)
{
  installer_t   *installer = udata;

  if (installer->guienabled) {
    g_application_quit (G_APPLICATION (installer->app));
  } else {
    installer->instState = INST_FINISH;
  }

  return;
}

static void
installerInstall (GtkButton *b, gpointer udata)
{
  installer_t *installer = udata;

  installer->instState = INST_INIT;
}

static bool
installerCheckTarget (installer_t *installer, const char *dir)
{
  char        tbuff [MAXPATHLEN];
  bool        exists;

  installerSetrundir (installer, dir);

  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4.exe", installer->rundir);
  } else {
    snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4", installer->rundir);
  }
  exists = fileopFileExists (tbuff);
  if (exists) {
    installer->newinstall = false;
  } else {
    installer->newinstall = true;
  }

  return exists;
}

/* installation routines */

static void
installerDelay (installer_t *installer)
{
  installer->instState = INST_DELAY;
  ++installer->delayCount;
  if (installer->delayCount > 50) {
    installer->instState = installer->delayState;
  }
}

static void
installerInstInit (installer_t *installer)
{
  bool        exists = false;
  char        tbuff [MAXPATHLEN];

  if (installer->guienabled) {
    installer->target = strdup (gtk_entry_buffer_get_text (installer->targetBuffer));
    installer->freetarget = true;
    installer->reinstall = gtk_toggle_button_get_active (
        GTK_TOGGLE_BUTTON (installer->reinstWidget));

    if (installer->bdj3loc != NULL && installer->freebdj3loc) {
      free (installer->bdj3loc);
    }
    installer->bdj3loc = strdup (gtk_entry_buffer_get_text (installer->bdj3locBuffer));
    installer->freebdj3loc = true;
  }

  if (! installer->guienabled) {
    tbuff [0] = '\0';
    printf (_("Enter the destination folder."));
    printf ("\n");
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
        printf (_("Overwriting existing BDJ4 installation."));
      } else {
        printf (_("Updating existing BDJ4 installation."));
      }
      printf ("\n");
      fflush (stdout);

      printf (_("Proceed with installation?"));
      printf ("\n");
      printf ("[Y] : ");
      fflush (stdout);
      fgets (tbuff, sizeof (tbuff), stdin);
      stringTrim (tbuff);
      if (*tbuff != '\0') {
        if (strncmp (tbuff, "Y", 1) != 0 &&
            strncmp (tbuff, "y", 1) != 0) {
          printf (" * %s", _("Installation aborted."));
          printf ("\n");
          fflush (stdout);
          installer->instState = INST_BEGIN;
          return;
        }
      }
    }

    if (! exists) {
      printf (_("New BDJ4 installation."));

      /* do not allow an overwrite of an existing directory that is not bdj4 */
      if (installer->guienabled) {
        gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), _("Folder already exists."));
      }

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

  installerDisplayText (installer, "-- ", _("Saving install location."));

  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "%s/AppData/Roaming/BDJ4", installer->home);
  } else {
    snprintf (tbuff, sizeof (tbuff), "%s/.config/BDJ4", installer->home);
  }
  fileopMakeDir (tbuff);

  installerGetTargetFname (installer, tbuff, sizeof (tbuff));

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
  installerDisplayText (installer, "-- ", _("Copying files."));

  /* the unpackdir is not necessarily the same as the current dir */
  /* on mac os, they are different */
  if (chdir (installer->unpackdir) < 0) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->unpackdir);
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
  FILE  *fh;
  char  tbuff [MAXPATHLEN];

  installerDisplayText (installer, "-- ", _("Cleaning old files."));

  if (chdir (installer->rundir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->rundir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  fh = fileopOpen ("install/cleanuplist.txt", "r");
  if (fh != NULL) {
    while (fgets (tbuff, sizeof (tbuff), fh) != NULL) {
      stringTrim (tbuff);
      if (! fileopFileExists (tbuff)) {
        continue;
      }

      if (fileopIsDirectory (tbuff)) {
        filemanipDeleteDir (tbuff);
      } else {
        fileopDelete (tbuff);
      }
    }
    fclose (fh);
  }

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
  datafile_t      *autodf;
  char            localesfx [20];
  slist_t         *renamelist;


  snprintf (localesfx, sizeof (localesfx), ".%s", sysvarsGetStr (SV_SHORT_LOCALE));
  renamelist = NULL;

  if (! installer->newinstall && ! installer->reinstall) {
    installer->instState = INST_CONVERT_START;
    return;
  }

  installerDisplayText (installer, "-- ", _("Copying template files."));

  if (chdir (installer->datatopdir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->datatopdir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%s/install/%s", installer->rundir,
      "localized-sr.txt");
  srdf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);
  snprintf (tbuff, sizeof (tbuff), "%s/install/%s", installer->rundir,
      "localized-auto.txt");
  autodf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL,
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
    if (pathInfoExtCheck (pi, ".crt")) {
      free (pi);
      continue;
    }
    if (pathInfoExtCheck (pi, ".html")) {
      free (pi);
      continue;
    }

    if (pathInfoExtCheck (pi, ".svg")) {
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
      }
      if (pathInfoExtCheck (pi, ".p")) {
        snprintf (to, sizeof (to), "data/profiles/%s", tbuff);
      }
      if (pathInfoExtCheck (pi, ".m")) {
        snprintf (to, sizeof (to), "data/%s/%s", installer->hostname, tbuff);
      }
      if (pathInfoExtCheck (pi, ".mp")) {
        snprintf (to, sizeof (to), "data/%s/profiles/%s",
            installer->hostname, tbuff);
      }
    } else if (pathInfoExtCheck (pi, ".txt") ||
        pathInfoExtCheck (pi, ".sequence") ||
        pathInfoExtCheck (pi, ".pldances") ||
        pathInfoExtCheck (pi, ".pl") ) {

      renamelist = NULL;
      if (strncmp (pi->basename, "automatic", pi->blen) == 0) {
        renamelist = datafileGetList (autodf);
      }
      if (strncmp (pi->basename, "standardrounds", pi->blen) == 0) {
        renamelist = datafileGetList (srdf);
      }

      strlcpy (tbuff, fname, sizeof (tbuff));
      if (renamelist != NULL) {
        char    *tval;

        tval = slistGetStr (renamelist, sysvarsGetStr (SV_SHORT_LOCALE));
        if (tval != NULL) {
          snprintf (tbuff, sizeof (tbuff), "%s%*s", tval, (int) pi->elen,
              pi->extension);
        }
      }

      snprintf (from, sizeof (from), "%s/templates/%s",
          installer->rundir, tbuff);
      snprintf (to, sizeof (to), "data/%s", tbuff);
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
    system (tbuff);
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
  char  buff [MAXPATHLEN];
  char  *locs [15];
  int   locidx = 0;
  FILE  *fh;

  if (! installer->newinstall && ! installer->reinstall) {
    installer->instState = INST_CREATE_SHORTCUT;
    return;
  }

  if (strcmp (installer->bdj3loc, "-") == 0) {
    installer->instState = INST_CREATE_SHORTCUT;
    return;
  }

  if (! installer->guienabled) {
    tbuff [0] = '\0';
    printf (_("Enter the folder where BallroomDJ 3 is installed."));
    printf ("\n");
    printf (_("The conversion process will only run for new installations and for re-installs."));
    printf ("\n");
    printf (_("Press 'Enter' to select the default."));
    printf ("\n");
    printf (_("If there is no BallroomDJ 3 installation, enter a single '-'."));
    printf ("\n");
    printf (_("BallroomDJ 3 Folder [%s] : "), installer->bdj3loc);
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

  if (strcmp (installer->bdj3loc, "-") == 0) {
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

  installerDisplayText (installer, "-- ", _("Starting conversion process."));

  fh = fileopOpen (tbuff, "w");
  if (fh != NULL) {
    fprintf (fh, "%s\n", installer->bdj3loc);
    fclose (fh);
  }

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
  locs [locidx++] = strdup ("/opt/local/bin/tclsh");
  locs [locidx++] = strdup ("/usr/local/bin/tclsh");
  locs [locidx++] = strdup ("/usr/bin/tclsh");
  locs [locidx++] = NULL;

  locidx = 0;
  while (locs [locidx] != NULL) {
    strlcpy (tbuff, locs [locidx], MAXPATHLEN);
    if (isWindows ()) {
      snprintf (tbuff, sizeof (tbuff), "%s.exe", locs [locidx]);
    }

    if (installer->tclshloc == NULL && fileopFileExists (tbuff)) {
      strlcpy (buff, tbuff, MAXPATHLEN);
      if (isWindows ()) {
        pathToWinPath (buff, tbuff, MAXPATHLEN);
      }
      installer->tclshloc = strdup (buff);
      installerDisplayText (installer, "   ", _("Located 'tclsh'."));
    }

    free (locs [locidx]);
    ++locidx;
  }

  if (installer->tclshloc == NULL) {
    installerDisplayText (installer, "   ", _("Unable to locate 'tclsh'."));
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
  if (*url) {
    snprintf (tbuff, sizeof (tbuff), "curl --location --remote-name "
        "--silent --user-agent BDJ4 %s", url);
    system (tbuff);
  }

  if (fileopFileExists (installer->dlfname)) {
    snprintf (tbuff, sizeof (tbuff), _("Installing %s."), "VLC");
    installerDisplayText (installer, "-- ", tbuff);
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
    snprintf (tbuff, sizeof (tbuff), _("%s installed."), "VLC");
    installerDisplayText (installer, "-- ", tbuff);
  }
  fileopDelete (installer->dlfname);
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
  if (*url) {
    snprintf (tbuff, sizeof (tbuff), "curl --location --remote-name "
        "--silent --user-agent BDJ4 %s", url);
    system (tbuff);
  }

  if (fileopFileExists (installer->dlfname)) {
    snprintf (tbuff, sizeof (tbuff), _("Installing %s."), "Python");
    installerDisplayText (installer, "-- ", tbuff);
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
    snprintf (tbuff, sizeof (tbuff), _("%s installed."), "Python");
    installerDisplayText (installer, "-- ", tbuff);
  }
  fileopDelete (installer->dlfname);
  installer->instState = INST_MUTAGEN_CHECK;
}

static void
installerMutagenCheck (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];

  if (chdir (installer->datatopdir)) {
    fprintf (stderr, "Unable to set working dir: %s\n", installer->datatopdir);
    installerDisplayText (installer, "", _("Error: Unable to set working folder."));
    installerDisplayText (installer, " * ", _("Installation aborted."));
    installer->instState = INST_BEGIN;
    return;
  }

  snprintf (tbuff, sizeof (tbuff), _("Installing %s."), "Mutagen");
  installerDisplayText (installer, "-- ", tbuff);
  installer->delayCount = 0;
  installer->delayState = INST_MUTAGEN_INSTALL;
  installer->instState = INST_DELAY;
}

static void
installerMutagenInstall (installer_t *installer)
{
  char      tbuff [MAXPATHLEN];

  strlcpy (tbuff, "pip install --no-cache-dir --user --upgrade mutagen", sizeof (tbuff));
  system (tbuff);
  snprintf (tbuff, sizeof (tbuff), _("%s installed."), "Mutagen");
  installer->instState = INST_FINISH;
}

/* support routines */

static void
installerCleanup (installer_t *installer)
{
  char  buff [MAXPATHLEN];
  char  *targv [10];

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
installerGetTargetFname (installer_t *installer, char *buff, size_t sz)
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
  char      localesfx [20];
  char      tbuff [MAXPATHLEN];

  snprintf (localesfx, sizeof (localesfx), ".%s", sysvarsGetStr (SV_LOCALE));
  strlcpy (tbuff, from, MAXPATHLEN);
  strlcat (tbuff, localesfx, MAXPATHLEN);
  if (fileopFileExists (tbuff)) {
    from = tbuff;
  } else {
    snprintf (localesfx, sizeof (localesfx), ".%s", sysvarsGetStr (SV_SHORT_LOCALE));
    strlcpy (tbuff, from, MAXPATHLEN);
    strlcat (tbuff, localesfx, MAXPATHLEN);
    if (fileopFileExists (tbuff)) {
      from = tbuff;
    }
  }
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
  char      *data;
  char      *p;
  char      *e;
  char      *tmpfile = "tmp/vlcvers.txt";
  char      tbuff [MAXPATHLEN];

  *installer->vlcversion = '\0';
  snprintf (tbuff, sizeof (tbuff),
      "curl --silent --user-agent BDJ4 -o %s "
      "http://get.videolan.org/vlc/last/macosx/", tmpfile);
  system (tbuff);
  if (fileopFileExists (tmpfile)) {
    data = filedataReadAll (tmpfile);

    /* vlc-3.0.16-intel64.dmg */
    p = strstr (data, "vlc-");
    p += 4;
    e = strstr (p, "-");
    strlcpy (installer->vlcversion, p, e - p + 1);
  }
}

static void
installerPythonGetVersion (installer_t *installer)
{
  char      *data;
  char      *p;
  char      *e;
  char      *tmpfile = "tmp/pyvers.txt";
  char      tbuff [MAXPATHLEN];

  *installer->pyversion = '\0';
  snprintf (tbuff, sizeof (tbuff),
      "curl --silent --user-agent BDJ4 -o %s "
      "https://www.python.org/downloads/windows/", tmpfile);
  system (tbuff);
  if (fileopFileExists (tmpfile)) {
    data = filedataReadAll (tmpfile);

    p = strstr (data, "Release - Python ");
    p += 17;
    e = strstr (p, "<");
    strlcpy (installer->pyversion, p, e - p + 1);
  }
}
