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
#include "log.h"
#include "pathutil.h"
#include "portability.h"
#include "sysvars.h"
#include "tmutil.h"
#include "uiutils.h"

typedef struct {
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
} installer_t;

#define INST_TEMP_FILE  "tmp/bdj4instout.txt"
#define CONV_RUN_FILE   "install/convrun.txt"

static int  installerCreateGui (installer_t *installer, int argc, char *argv []);
static void installerActivate (GApplication *app, gpointer udata);
static int  installerMainLoop (void *udata);
static void installerSelectDirDialog (GtkButton *b, gpointer udata);
static void installerExit (GtkButton *b, gpointer udata);
static void installerValidateDir (installer_t *installer, bool okonly);
static void installerValidateStart (GtkEditable *e, gpointer udata);
static void installerInstall (GtkButton *b, gpointer udata);
static void installerSaveTargetDir (installer_t *installer);
static void installerCopyFiles (installer_t *installer);
static void installerCleanOldFiles (installer_t *installer);
static void installerCopyTemplates (installer_t *installer);
static void installerRunConversion (installer_t *installer);
static void installerDisplayText (installer_t *installer, char *txt);
static void installerScrollToEnd (GtkWidget *w, GtkAllocation *retAllocSize, gpointer udata);;

int
main (int argc, char *argv[])
{
  installer_t   installer;
  int           status;
  char          *home;
  char          tbuff [512];
  char          buff [512];


  buff [0] = '\0';
  home = getenv ("HOME");
  if (home == NULL) {
    /* probably a windows machine */
    home = getenv ("USERPROFILE");
  }

  snprintf (tbuff, sizeof (tbuff), "%s/BDJ4", home);
  strlcpy (buff, tbuff, sizeof (buff));
  if (isMacOS ()) {
    snprintf (buff, sizeof (buff), "%s/Applications/BallroomDJ4.app", home);
  }
  if (isWindows ()) {
    pathToWinPath (buff, tbuff, sizeof (tbuff));
  }

  if (! isWindows()) {
//    snprintf (tbuff, sizeof (tbuff), "%s/.config/BDJ4/installdir.txt", home);
  }

  installer.target = buff;
  installer.app = NULL;
  installer.window = NULL;
  installer.targetBuffer = NULL;
  installer.targetEntry = NULL;
  installer.dispBuffer = NULL;
  mstimeset (&installer.validateTimer, 3600000);
  getcwd (installer.unpackdir, MAXPATHLEN);
  installer.newinstall = true;
  installer.reinstall = false;

  /* for convenience in testing; normally will already be there */
  sysvarsInit (argv [0]);
  if (chdir (sysvarsGetStr (SV_BDJ4DIR)) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvarsGetStr (SV_BDJ4DIR));
    exit (1);
  }

  logStartAppend ("bdj4installer", "cv", LOG_IMPORTANT);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "conversion-start");

  /* initial location */
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "initial target: %s", installer.target);

  g_timeout_add (UI_MAIN_LOOP_TIMER, installerMainLoop, &installer);
  status = installerCreateGui (&installer, 0, NULL);

  if (installer.target != NULL && *installer.target) {
// need to do this if using gnu info?
//    free (installer.target);
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
    installerValidateDir (installer, false);
    mstimeset (&installer->validateTimer, 3600000);
  }

  return TRUE;
}

static void
installerValidateDir (installer_t *installer, bool okonly)
{
  struct stat   statbuf;
  int           rc;
  bool          locok = false;
  const char    *fn;
  bool          exists = false;

  fn = gtk_entry_buffer_get_text (installer->targetBuffer);

  gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), "");
  exists = fileopExists (fn);
  if (exists) {
    gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), _("Folder already exists."));
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
  mstimeset (&installer->validateTimer, 700);
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
  bool        exists = false;
  char        tbuff [MAXPATHLEN];

  installer->target = strdup (gtk_entry_buffer_get_text (installer->targetBuffer));
  installer->reinstall = gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (installer->reinstWidget));
  exists = fileopExists (installer->target);
  if (exists) {
    if (isWindows ()) {
      snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4.exe", installer->target);
    } else {
      snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4", installer->target);
    }
    exists = fileopExists (tbuff);
    if (exists) {
      installer->newinstall = false;
    }

    /* do not allow an overwrite of an existing directory that is not bdj4 */
    if (installer->newinstall) {
      gtk_label_set_text (GTK_LABEL (installer->feedbackMsg), _("Folder already exists."));

      snprintf (tbuff, sizeof (tbuff), _("Folder %s already exists."),
          installer->target);
      installerDisplayText (installer, tbuff);
      installerDisplayText (installer, _("Stopped"));
      return;
    }
  }

  if (installer->reinstall) {
    installer->newinstall = true;
  }

  installerDisplayText (installer, _("Saving install location."));
  installerSaveTargetDir (installer);

  installerDisplayText (installer, _("Creating folder structure."));
  /* create the directories that are not included in the distribution */
  fileopMakeDir ("data");
  /* this will create the directories necessary for the configs */
  bdjoptCreateDirectories ();
  fileopMakeDir ("tmp");
  fileopMakeDir ("http");

  installerDisplayText (installer, _("Copying files."));
  installerCopyFiles (installer);
  installerDisplayText (installer, _("  Copy finished."));

  fileopMakeDir (installer->target);

  if (chdir (installer->target)) {
    fprintf (stderr, "Unable to chdir: %s\n", installer->target);
    installerDisplayText (installer, _("Unable to set working directory."));
    installerDisplayText (installer, _("Stopped"));
    return;
  }

  installerDisplayText (installer, _("Cleaning old files."));
  installerCleanOldFiles (installer);
  installerDisplayText (installer, _("Copying template files."));
  installerCopyTemplates (installer);

  installerRunConversion (installer);

  return;
}

static void
installerSaveTargetDir (installer_t *installer)
{
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
        "tar -c -f - . | (cd \"%s\"; tar -x -f -)",
        installer->target);
    system (tbuff);
  }
}

static void
installerCleanOldFiles (installer_t *installer)
{
}

static void
installerCopyTemplates (installer_t *installer)
{
}

static void
installerRunConversion (installer_t *installer)
{
  char      tbuff [MAXPATHLEN];

  if (installer->reinstall) {
    fileopDelete (CONV_RUN_FILE);
  }

  if (installer->newinstall &&
      ! fileopExists (CONV_RUN_FILE)) {
    installerDisplayText (installer, _("Starting conversion process."));
    strlcpy (tbuff, "./bin/bdj4converter", MAXPATHLEN);
    if (isWindows ()) {
      strlcat (tbuff, ".exe", MAXPATHLEN);
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
  } else {
    installerDisplayText (installer, _("Conversion already done."));
  }
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
