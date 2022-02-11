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

#include <gtk/gtk.h>

#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "fileutil.h"
#include "localeutil.h"
#include "log.h"
#include "pathbld.h"
#include "pathutil.h"
#include "portability.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"
#include "uiutils.h"

typedef enum {
  INST_BEGIN,
  INST_INIT,
  INST_SAVE,
  INST_MAKE_TARGET,
  INST_COPY_START,
  INST_COPY_FILES,
  INST_CHDIR,
  INST_CREATE_DIRS,
  INST_CLEAN,
  INST_COPY_TEMPLATES,
  INST_CONV_START,
  INST_CONVERTER,
  INST_FINISH,
} installstate_t;

typedef struct {
  installstate_t  instState;
  char            *home;
  char            *target;
  char            unpackdir [MAXPATHLEN];
  GtkApplication  *app;
  GtkWidget       *window;
  GtkWidget       *targetEntry;
  GtkWidget       *reinstWidget;
  GtkWidget       *feedbackMsg;
  GtkEntryBuffer  *targetBuffer;
  GtkTextBuffer   *dispBuffer;
  GtkWidget       *dispTextView;
  mstime_t        validateTimer;
  bool            newinstall : 1;
  bool            reinstall : 1;
  bool            freetarget : 1;
} installer_t;

#define INST_TEMP_FILE  "tmp/bdj4instout.txt"
#define CONV_RUN_FILE   "install/convrun.txt"
#define INST_SAVE_FNAME "installdir.txt"

static int  installerCreateGui (installer_t *installer, int argc, char *argv []);
static void installerActivate (GApplication *app, gpointer udata);
static int  installerMainLoop (void *udata);
static void installerSelectDirDialog (GtkButton *b, gpointer udata);
static void installerExit (GtkButton *b, gpointer udata);
static void installerCheckDir (GtkButton *b, gpointer udata);
static void installerValidateDir (installer_t *installer);
static void installerValidateStart (GtkEditable *e, gpointer udata);
static void installerInstall (GtkButton *b, gpointer udata);
static bool installerCheckTarget (installer_t *installer);
static void installerInstInit (installer_t *installer);
static void installerSaveTargetDir (installer_t *installer);
static void installerMakeTarget (installer_t *installer);
static void installerCopyStart (installer_t *installer);
static void installerCopyFiles (installer_t *installer);
static void installerChangeDir (installer_t *installer);
static void installerCreateDirs (installer_t *installer);
static void installerCleanOldFiles (installer_t *installer);
static void installerCopyTemplates (installer_t *installer);
static void installerRunConversion (installer_t *installer);
static void installerDisplayText (installer_t *installer, char *txt);
static void installerScrollToEnd (GtkWidget *w, GtkAllocation *retAllocSize, gpointer udata);;
static void installerSaveTargetFname (installer_t *installer, char *buff, size_t len);
static void installerTemplateCopy (char *from, char *to);

int
main (int argc, char *argv[])
{
  installer_t   installer;
  int           status;
  char          tbuff [512];
  char          buff [MAXPATHLEN];
  FILE          *fh;


  localeInit ();

  buff [0] = '\0';
  installer.home = getenv ("HOME");
  if (installer.home == NULL) {
    /* probably a windows machine */
    installer.home = getenv ("USERPROFILE");
  }

  snprintf (tbuff, sizeof (tbuff), "%s/BDJ4", installer.home);
  strlcpy (buff, tbuff, sizeof (buff));
  if (isMacOS ()) {
    snprintf (buff, sizeof (buff), "%s/Applications/BallroomDJ4.app", installer.home);
  }

  installer.instState = INST_BEGIN;
  installer.target = buff;
  installer.app = NULL;
  installer.window = NULL;
  installer.targetBuffer = NULL;
  installer.feedbackMsg = NULL;
  installer.reinstWidget = NULL;
  installer.targetEntry = NULL;
  installer.dispBuffer = NULL;
  /* want to do an initial validation */
  mstimeset (&installer.validateTimer, 500);
  getcwd (installer.unpackdir, MAXPATHLEN);
  installer.newinstall = true;
  installer.reinstall = false;
  installer.freetarget = false;

  /* for convenience in testing; normally will already be there */
  sysvarsInit (argv [0]);
  if (chdir (sysvarsGetStr (SV_BDJ4DIR)) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvarsGetStr (SV_BDJ4DIR));
    exit (1);
  }

  installerSaveTargetFname (&installer, tbuff, sizeof (tbuff));
  fh = fopen (tbuff, "r");
  if (fh != NULL) {
    /* installer.target is pointing at buff */
    fgets (buff, MAXPATHLEN, fh);
    stringTrim (buff);
    fclose (fh);
  }

  if (isWindows ()) {
    /* installer.target is pointing at buff */
    strlcpy (tbuff, installer.target, sizeof (tbuff));
    pathToWinPath (installer.target, tbuff, sizeof (tbuff));
  }

  g_timeout_add (UI_MAIN_LOOP_TIMER, installerMainLoop, &installer);
  status = installerCreateGui (&installer, 0, NULL);

  if (installer.freetarget && installer.target != NULL) {
    free (installer.target);
  }
  logEnd ();

  return status;
}

static int
installerCreateGui (installer_t *installer, int argc, char *argv [])
{
  int             status;

  installer->app = gtk_application_new (
      "org.ballroomdj.BallroomDJ.installer",
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
  GError                *gerr;


  window = gtk_application_window_new (GTK_APPLICATION (app));

  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_window_set_focus_on_map (GTK_WINDOW (window), FALSE);
  gtk_window_set_title (GTK_WINDOW (window), _("BDJ4 installer"));
  gtk_window_set_default_icon_from_file ("img/bdj4_icon.svg", &gerr);
  gtk_window_set_default_size (GTK_WINDOW (window), 1000, 600);
  installer->window = window;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_margin_top (GTK_WIDGET (vbox), 20);
  gtk_widget_set_margin_bottom (GTK_WIDGET (vbox), 10);
  gtk_widget_set_margin_start (GTK_WIDGET (vbox), 10);
  gtk_widget_set_margin_end (GTK_WIDGET (vbox), 10);
  gtk_widget_set_hexpand (GTK_WIDGET (vbox), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (vbox), TRUE);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

  widget = gtk_label_new (_("Enter the destination folder where BallroomDJ 4 will be installed."));
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

  installer->targetBuffer = gtk_entry_buffer_new (installer->target, -1);
  installer->targetEntry = gtk_entry_new_with_buffer (installer->targetBuffer);
  gtk_entry_set_width_chars (GTK_ENTRY (installer->targetEntry), 80);
  gtk_entry_set_max_length (GTK_ENTRY (installer->targetEntry), MAXPATHLEN);
  gtk_entry_set_input_purpose (GTK_ENTRY (installer->targetEntry), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_halign (installer->targetEntry, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (GTK_WIDGET (installer->targetEntry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (installer->targetEntry),
      TRUE, TRUE, 0);
  g_signal_connect (installer->targetEntry, "changed", G_CALLBACK (installerValidateStart), installer);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

  installer->reinstWidget = gtk_check_button_new_with_label (_("Re-Install"));
  gtk_box_pack_start (GTK_BOX (hbox), installer->reinstWidget, FALSE, FALSE, 0);
  g_signal_connect (installer->reinstWidget, "toggled",
      G_CALLBACK (installerCheckDir), installer);

  installer->feedbackMsg = gtk_label_new ("");
  uiutilsSetCss (GTK_WIDGET (installer->feedbackMsg),
      "label { color: #ffa600; }");
  gtk_widget_set_halign (installer->feedbackMsg, GTK_ALIGN_END);
  gtk_box_pack_end (GTK_BOX (hbox), installer->feedbackMsg, FALSE, FALSE, 0);

  /* button box */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Exit"));
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked", G_CALLBACK (installerExit), installer);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Install"));
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked", G_CALLBACK (installerInstall), installer);

  scwidget = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (scwidget), FALSE);
  gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (scwidget), TRUE);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (scwidget), TRUE);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scwidget), 500);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scwidget), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand (GTK_WIDGET (scwidget), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (scwidget), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scwidget),
      FALSE, FALSE, 0);

  installer->dispBuffer = gtk_text_buffer_new (NULL);
  installer->dispTextView = gtk_text_view_new_with_buffer (installer->dispBuffer);
  gtk_widget_set_size_request (installer->dispTextView, -1, 400);
  gtk_widget_set_can_focus (installer->dispTextView, FALSE);
  gtk_widget_set_halign (installer->dispTextView, GTK_ALIGN_FILL);
  gtk_widget_set_valign (installer->dispTextView, GTK_ALIGN_START);
  gtk_widget_set_hexpand (installer->dispTextView, TRUE);
  gtk_widget_set_vexpand (installer->dispTextView, TRUE);
  gtk_container_add (GTK_CONTAINER (scwidget), GTK_WIDGET (installer->dispTextView));

  /* push the text view to the top */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_vexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), TRUE, TRUE, 0);

  gtk_widget_show_all (GTK_WIDGET (window));
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
    case INST_CONV_START: {
      /* want to give gtk some time to update the display */
      if (installer->newinstall &&
          ! fileopExists (CONV_RUN_FILE)) {
        installerDisplayText (installer, _("-- Starting conversion process."));
      }
      installer->instState = INST_CONVERTER;
      break;
    }
    case INST_CONVERTER: {
      installerRunConversion (installer);
      break;
    }
    case INST_FINISH: {
      installerDisplayText (installer, _("## Installation complete."));
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
  struct stat   statbuf;
  int           rc;
  bool          locok = false;
  const char    *fn;
  bool          exists = false;

  if (installer->feedbackMsg == NULL ||
      installer->targetBuffer == NULL ||
      installer->reinstWidget == NULL) {
    mstimeset (&installer->validateTimer, 500);
    return;
  }

  fn = gtk_entry_buffer_get_text (installer->targetBuffer);

  installer->reinstall = gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (installer->reinstWidget));
  gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), "");
  exists = fileopExists (fn);
  if (exists && ! installer->reinstall) {
    exists = installerCheckTarget (installer);
    if (exists) {
      gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), _("BDJ4 folder already exists."));
    } else {
      gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), _("Error: Existing folder."));
    }
  } else {
    gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), "");
  }
}

static void
installerValidateStart (GtkEditable *e, gpointer udata)
{
  installer_t   *installer = udata;

  /* if the user is typing, clear the message */
  gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), "");
  mstimeset (&installer->validateTimer, 500);
}

static void
installerExit (GtkButton *b, gpointer udata)
{
  installer_t   *installer = udata;

  g_application_quit (G_APPLICATION (installer->app));
  return;
}

static void
installerInstall (GtkButton *b, gpointer udata)
{
  installer_t *installer = udata;

  installer->instState = INST_INIT;
}

static bool
installerCheckTarget (installer_t *installer)
{
  char        tbuff [MAXPATHLEN];
  bool        exists;
  const char  *fn;

  fn = gtk_entry_buffer_get_text (installer->targetBuffer);

  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4.exe", fn);
  } else {
    snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4", fn);
  }
  exists = fileopExists (tbuff);
  if (exists) {
    installer->newinstall = false;
  }

  return exists;
}

static void
installerInstInit (installer_t *installer)
{
  bool        exists = false;
  char        tbuff [MAXPATHLEN];

  installer->target = strdup (gtk_entry_buffer_get_text (installer->targetBuffer));
  installer->freetarget = true;
  installer->reinstall = gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (installer->reinstWidget));
  exists = fileopExists (installer->target);
  if (exists) {
    exists = installerCheckTarget (installer);

    /* do not allow an overwrite of an existing directory that is not bdj4 */
    if (installer->newinstall) {
      gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), _("Folder already exists."));

      snprintf (tbuff, sizeof (tbuff), _("Error: Folder %s already exists."),
          installer->target);
      installerDisplayText (installer, tbuff);
      installerDisplayText (installer, _(" * Stopped"));
      installer->instState = INST_BEGIN;
      return;
    }
  }

  if (installer->reinstall) {
    installer->newinstall = true;
  }

  if (chdir (installer->unpackdir) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", installer->unpackdir);
    installerDisplayText (installer, _("Error: Unable to set working directory."));
    installerDisplayText (installer, _(" * Stopped"));
    installer->instState = INST_BEGIN;
    return;
  }

  installer->instState = INST_SAVE;
}

static void
installerSaveTargetDir (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];
  FILE  *fh;

  installerDisplayText (installer, _("-- Saving install location."));

  if (isWindows ()) {
    snprintf (tbuff, MAXPATHLEN, "%s/AppData/Roaming/BDJ4", installer->home);
  } else {
    snprintf (tbuff, MAXPATHLEN, "%s/.config/BDJ4", installer->home);
  }
  fileopMakeDir (tbuff);

  installerSaveTargetFname (installer, tbuff, MAXPATHLEN);

  fh = fopen (tbuff, "w");
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

  installer->instState = INST_COPY_START;
}

static void
installerCopyStart (installer_t *installer)
{
  installerDisplayText (installer, _("-- Copying files."));

  installer->instState = INST_COPY_FILES;
}

static void
installerCopyFiles (installer_t *installer)
{
  char      tbuff [MAXPATHLEN];

  if (isWindows ()) {
    snprintf (tbuff, MAXPATHLEN,
        "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl . \"%s\"",
        installer->target);
    system (tbuff);
  } else {
    snprintf (tbuff, MAXPATHLEN,
        "tar -c -f - . | (cd '%s'; tar -x -f -)",
        installer->target);
    system (tbuff);
  }
  installerDisplayText (installer, _("   Copy finished."));
  installer->instState = INST_CHDIR;
}

static void
installerChangeDir (installer_t *installer)
{
  if (chdir (installer->target)) {
    fprintf (stderr, "Unable to chdir: %s\n", installer->target);
    installerDisplayText (installer, _("Error: Unable to set working directory."));
    installerDisplayText (installer, _(" * Stopped"));
    installer->instState = INST_BEGIN;
    return;
  }
  installer->instState = INST_CREATE_DIRS;
}

static void
installerCreateDirs (installer_t *installer)
{
  installerDisplayText (installer, _("-- Creating folder structure."));
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

  installerDisplayText (installer, _("-- Cleaning old files."));

  fh = fopen ("install/cleanuplist.txt", "r");
  if (fh != NULL) {
    while (fgets (tbuff, MAXPATHLEN, fh) != NULL) {
      if (! fileopExists (tbuff)) {
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


  installerDisplayText (installer, _("-- Copying template files."));

  dirlist = filemanipBasicDirList ("templates", NULL);
  slistStartIterator (dirlist, &iteridx);
  while ((fname = slistIterateKey (dirlist, &iteridx)) != NULL) {
    if (fileopIsDirectory (fname)) {
      continue;
    }
    if (strcmp (fname, "qrcode.html") == 0) {
      continue;
    }
    if (strcmp (fname, "html-list.txt") == 0) {
      continue;
    }

    if (strcmp (fname, "bdj-flex-dark.html") == 0) {
      pathbldMakePath (from, MAXPATHLEN, "", fname, "", PATHBLD_MP_TEMPLATEDIR);
      pathbldMakePath (to, MAXPATHLEN, "", "bdj4remote", ".html", PATHBLD_MP_HTTPDIR);
      installerTemplateCopy (from, to);
      continue;
    }
    if (strcmp (fname, "mobilemq.html") == 0) {
      pathbldMakePath (from, MAXPATHLEN, "", fname, "", PATHBLD_MP_TEMPLATEDIR);
      pathbldMakePath (to, MAXPATHLEN, "", fname, "", PATHBLD_MP_HTTPDIR);
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
      pathbldMakePath (from, MAXPATHLEN, "", fname, "", PATHBLD_MP_TEMPLATEDIR);
      pathbldMakePath (to, MAXPATHLEN, "", fname, "",
          PATHBLD_MP_RELATIVE | PATHBLD_MP_IMGDIR);
    } else if (strncmp (fname, "bdjconfig", 9) == 0) {
      pathbldMakePath (from, MAXPATHLEN, "", fname, "", PATHBLD_MP_TEMPLATEDIR);

      snprintf (tbuff, MAXPATHLEN, "%.*s", (int) pi->blen, pi->basename);
      if (pathInfoExtCheck (pi, ".g")) {
        pathbldMakePath (to, MAXPATHLEN, "", tbuff, "",
            PATHBLD_MP_RELATIVE);
      }
      if (pathInfoExtCheck (pi, ".p")) {
        pathbldMakePath (to, MAXPATHLEN, "profiles", tbuff, "",
            PATHBLD_MP_RELATIVE);
      }
      if (pathInfoExtCheck (pi, ".m")) {
        pathbldMakePath (to, MAXPATHLEN, "", tbuff, "",
            PATHBLD_MP_RELATIVE | PATHBLD_MP_HOSTNAME);
      }
      if (pathInfoExtCheck (pi, ".mp")) {
        pathbldMakePath (to, MAXPATHLEN, "profiles", tbuff, "",
            PATHBLD_MP_RELATIVE | PATHBLD_MP_HOSTNAME);
      }
    } else if (pathInfoExtCheck (pi, ".txt") ||
        pathInfoExtCheck (pi, ".sequence") ||
        pathInfoExtCheck (pi, ".pldances") ||
        pathInfoExtCheck (pi, ".pl") ) {
      pathbldMakePath (from, MAXPATHLEN, "", fname, "", PATHBLD_MP_TEMPLATEDIR);
      pathbldMakePath (to, MAXPATHLEN, "", fname, "", PATHBLD_MP_RELATIVE);
    }

    installerTemplateCopy (from, to);

    free (pi);
  }
  slistFree (dirlist);

  pathbldMakePath (from, MAXPATHLEN, "", "favicon.ico", "", PATHBLD_MP_IMGDIR);
  pathbldMakePath (to, MAXPATHLEN, "", "favicon.ico", "", PATHBLD_MP_HTTPDIR);
  installerTemplateCopy (from, to);

  pathbldMakePath (from, MAXPATHLEN, "", "led_on.svg", "", PATHBLD_MP_IMGDIR);
  pathbldMakePath (to, MAXPATHLEN, "", "led_on.svg", "", PATHBLD_MP_HTTPDIR);
  installerTemplateCopy (from, to);

  pathbldMakePath (from, MAXPATHLEN, "", "led_off.svg", "", PATHBLD_MP_IMGDIR);
  pathbldMakePath (to, MAXPATHLEN, "", "led_off.svg", "", PATHBLD_MP_HTTPDIR);
  installerTemplateCopy (from, to);

  pathbldMakePath (from, MAXPATHLEN, "", "ballroomdj.svg", "", PATHBLD_MP_IMGDIR);
  pathbldMakePath (to, MAXPATHLEN, "", "ballroomdj.svg", "", PATHBLD_MP_HTTPDIR);
  installerTemplateCopy (from, to);

  pathbldMakePath (from, MAXPATHLEN, "", "mrc", "", PATHBLD_MP_IMGDIR);
  pathbldMakePath (to, MAXPATHLEN, "", "", "", PATHBLD_MP_HTTPDIR);
  if (isWindows ()) {
    snprintf (tbuff, MAXPATHLEN,
        "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl \"%s\" \"%s\"",
        from, "http/mrc");
fprintf (stderr, "cmd: %s\n", tbuff);
    system (tbuff);
  } else {
    snprintf (tbuff, MAXPATHLEN, "cp -r '%s' '%s'", from, "http");
    system (tbuff);
  }

  installer->instState = INST_CONV_START;
}

static void
installerRunConversion (installer_t *installer)
{
  char      tbuff [MAXPATHLEN];
  char      buff [MAXPATHLEN];

  if (installer->reinstall) {
    fileopDelete (CONV_RUN_FILE);
  }

  if (installer->newinstall &&
      ! fileopExists (CONV_RUN_FILE)) {
    installerDisplayText (installer, _("-- Starting conversion process."));
    strlcpy (tbuff, "./bin/bdj4converter", MAXPATHLEN);
    if (isWindows ()) {
      strlcat (tbuff, ".exe", MAXPATHLEN);
      pathToWinPath (buff, tbuff, MAXPATHLEN);
      strlcpy (tbuff, buff, MAXPATHLEN);
    }
    strlcat (tbuff, " ", MAXPATHLEN);
    if (installer->newinstall) {
      strlcat (tbuff, "--newinstall ", MAXPATHLEN);
    }
    if (installer->reinstall) {
      strlcat (tbuff, "--reinstall ", MAXPATHLEN);
    }
    if (installer->reinstall) {
      strlcat (tbuff, "--targetdir ", MAXPATHLEN);
      strlcat (tbuff, installer->target, MAXPATHLEN);
      strlcat (tbuff, " ", MAXPATHLEN);
    }
    system (tbuff);
    installerDisplayText (installer, _("   Conversion complete."));
  } else {
    installerDisplayText (installer, _("-- Conversion already done."));
  }

  installer->instState = INST_FINISH;
}

static void
installerDisplayText (installer_t *installer, char *txt)
{
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter (installer->dispBuffer, &iter);
  gtk_text_buffer_insert (installer->dispBuffer, &iter, txt, -1);
  gtk_text_buffer_get_end_iter (installer->dispBuffer, &iter);
  gtk_text_buffer_insert (installer->dispBuffer, &iter, "\n", -1);
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
installerSaveTargetFname (installer_t *installer, char *buff, size_t sz)
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
installerTemplateCopy (char *from, char *to)
{
  char      localesfx [20];
  char      tbuff [MAXPATHLEN];

  snprintf (localesfx, sizeof (localesfx), ".%s", sysvarsGetStr (SV_LOCALE));
  strlcpy (tbuff, from, MAXPATHLEN);
  strlcat (tbuff, localesfx, MAXPATHLEN);
  if (fileopExists (tbuff)) {
    from = tbuff;
  }
  filemanipCopy (from, to);
}
