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

#include "bdjstring.h"
#include "log.h"
#include "portability.h"
#include "sysvars.h"
#include "tmutil.h"
#include "uiutils.h"

typedef struct {
  char            *loc;
  GtkApplication  *app;
  GtkWidget       *window;
  GtkWidget       *locEntry;
  GtkEntryBuffer  *locBuffer;
  GtkEntryBuffer  *dispBuffer;
  mstime_t        validateTimer;
} converter_t;

static int  converterCreateGui (converter_t *converter, int argc, char *argv []);
static void converterActivate (GApplication *app, gpointer udata);
static int  converterMainLoop (void *udata);
static void converterSelectDirDialog (GtkButton *b, gpointer udata);
static void converterExit (GtkButton *b, gpointer udata);
static void converterValidateDir (converter_t *converter, bool okonly);
static void converterValidateStart (GtkEditable *e, gpointer udata);
static void converterConvert (GtkButton *b, gpointer udata);
static char * locatebdj3 (void);
static bool locationcheck (char *dir);
static bool locatedb (char *dir);

int
main (int argc, char *argv[])
{
  converter_t   converter;
  int           status;


  converter.loc = "";
  converter.app = NULL;
  converter.window = NULL;
  converter.locBuffer = NULL;
  converter.locEntry = NULL;
  converter.dispBuffer = NULL;
  mstimeset (&converter.validateTimer, 3600000);

  /* for convenience in testing; normally will already be there */
  sysvarsInit (argv [0]);
  if (chdir (sysvarsGetStr (SV_BDJ4DIR)) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvarsGetStr (SV_BDJ4DIR));
    exit (1);
  }

  logStartAppend ("bdj4converter", "cv", LOG_IMPORTANT);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "conversion-start");

  /* initial location */
  converter.loc = locatebdj3 ();
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "initial loc: %s\n", converter.loc);

  g_timeout_add (UI_MAIN_LOOP_TIMER, converterMainLoop, &converter);
  status = converterCreateGui (&converter, 0, NULL);

  if (converter.loc != NULL && *converter.loc) {
    free (converter.loc);
  }
  logEnd ();

  return status;
}

static int
converterCreateGui (converter_t *converter, int argc, char *argv [])
{
  int             status;

  converter->app = gtk_application_new (
      "org.ballroomdj.BallroomDJ.converter",
      G_APPLICATION_FLAGS_NONE
  );
  g_signal_connect (converter->app, "activate", G_CALLBACK (converterActivate), converter);

  status = g_application_run (G_APPLICATION (converter->app), argc, argv);
  if (GTK_IS_WIDGET (converter->window)) {
    gtk_widget_destroy (converter->window);
  }
  g_object_unref (converter->app);
  return status;
}

static void
converterActivate (GApplication *app, gpointer udata)
{
  converter_t           *converter = udata;
  GtkWidget             *window;
  GtkWidget             *vbox;
  GtkWidget             *hbox;
  GtkWidget             *widget;
  GtkWidget             *image;
  GError                *gerr;


  window = gtk_application_window_new (GTK_APPLICATION (app));

  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_window_set_focus_on_map (GTK_WINDOW (window), FALSE);
  gtk_window_set_title (GTK_WINDOW (window), _("BDJ4 Converter"));
  gtk_window_set_default_icon_from_file ("img/bdj4_icon.svg", &gerr);
  gtk_window_set_default_size (GTK_WINDOW (window), 1000, 600);
  converter->window = window;

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

  widget = gtk_label_new (_("If this is a new BallroomDJ 4 installation, select 'Exit'."));
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

  widget = gtk_label_new (_("If you have already converted your BallroomDJ 3 data files, select 'Exit'."));
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

  widget = gtk_label_new (_("BallroomDJ 3 Location:"));
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);

  converter->locBuffer = gtk_entry_buffer_new (converter->loc, -1);
  converter->locEntry = gtk_entry_new_with_buffer (converter->locBuffer);
  gtk_entry_set_width_chars (GTK_ENTRY (converter->locEntry), 80);
  gtk_entry_set_max_length (GTK_ENTRY (converter->locEntry), MAXPATHLEN);
  gtk_entry_set_input_purpose (GTK_ENTRY (converter->locEntry), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_halign (converter->locEntry, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (GTK_WIDGET (converter->locEntry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (converter->locEntry),
      TRUE, TRUE, 0);
  g_signal_connect (converter->locEntry, "changed", G_CALLBACK (converterValidateStart), converter);

  widget = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_widget_set_halign (converter->locEntry, GTK_ALIGN_START);
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 0);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked", G_CALLBACK (converterSelectDirDialog), converter);

  /* button box */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Exit"));
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked", G_CALLBACK (converterExit), converter);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Convert"));
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked", G_CALLBACK (converterConvert), converter);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), TRUE, TRUE, 0);

  converter->dispBuffer = gtk_entry_buffer_new (NULL, 0);
  widget = gtk_entry_new_with_buffer (converter->dispBuffer);
  gtk_entry_set_input_purpose (GTK_ENTRY (converter->locEntry), GTK_INPUT_PURPOSE_TERMINAL);
  gtk_editable_set_editable (GTK_EDITABLE (widget), FALSE);
  gtk_widget_set_can_focus (widget, FALSE);
  gtk_widget_set_halign (widget, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (GTK_WIDGET (widget), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (widget), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), TRUE, TRUE, 0);

  gtk_widget_show_all (GTK_WIDGET (window));
}

static int
converterMainLoop (void *udata)
{
  converter_t *converter = udata;

  if (mstimeCheck (&converter->validateTimer)) {
    converterValidateDir (converter, false);
    mstimeset (&converter->validateTimer, 3600000);
  }

  return TRUE;
}

static void
converterSelectDirDialog (GtkButton *b, gpointer udata)
{
  converter_t           *converter = udata;
  GtkFileChooserNative  *widget = NULL;
  gint                  res;
  char                  *fn = NULL;

  widget = gtk_file_chooser_native_new (
      _("Select BallroomDJ 3 Location"),
      GTK_WINDOW (converter->window),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
      _("Select"), _("Close"));

  res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (widget));
  if (res == GTK_RESPONSE_ACCEPT) {
    fn = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
    if (converter->loc != NULL && *converter->loc) {
      free (converter->loc);
    }
    converter->loc = fn;
    gtk_entry_buffer_set_text (converter->locBuffer, fn, -1);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s\n", converter->loc);
  }

  g_object_unref (widget);
}

static void
converterValidateDir (converter_t *converter, bool okonly)
{
  struct stat   statbuf;
  int           rc;
  bool          locok = false;
  const char    *fn;

  fn = gtk_entry_buffer_get_text (converter->locBuffer);
  if (locationcheck (fn)) {
    locok = true;
  }

  if (! locok) {
    if (okonly == false) {
      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (converter->locEntry),
          GTK_ENTRY_ICON_SECONDARY, "dialog-error");
    }
  } else {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (converter->locEntry),
        GTK_ENTRY_ICON_SECONDARY, NULL);
  }
}

static void
converterValidateStart (GtkEditable *e, gpointer udata)
{
  converter_t   *converter = udata;

  /* if the user is typing, clear the icon */
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (converter->locEntry),
      GTK_ENTRY_ICON_SECONDARY, NULL);
  mstimeset (&converter->validateTimer, 500);
}

static void
converterExit (GtkButton *b, gpointer udata)
{
  converter_t   *converter = udata;

  g_application_quit (G_APPLICATION (converter->app));
  return;
}

static void
converterConvert (GtkButton *b, gpointer udata)
{
  return;
}

static char *
locatebdj3 (void)
{
  char          *loc;
  char          *home;
  struct stat   statbuf;
  char          tbuff [MAXPATHLEN];
  int           grc;
  int           rc;


  /* make it possible to specify a location via the environment */
  loc = getenv ("BDJ3_LOCATION");
  if (loc != NULL) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "from-env: %s\n", loc);
    if (locationcheck (loc)) {
      return strdup (loc);
    }
  }

  home = getenv ("HOME");
  if (home == NULL) {
    /* probably a windows machine */
    home = getenv ("USERPROFILE");
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "home: %s\n", loc);

  if (home == NULL) {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "err: no home env\n", loc);
    return "";
  }

  /* Linux, old MacOS, recent windows: $HOME/BallroomDJ */
  strlcpy (tbuff, home, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "BallroomDJ", MAXPATHLEN);

  if (locationcheck (tbuff)) {
    return strdup (tbuff);
  }

  /* windows: %USERPROFILE%/Desktop/BallroomDJ */
  strlcpy (tbuff, home, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "Desktop", MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "BallroomDJ", MAXPATHLEN);

  if (locationcheck (tbuff)) {
    return strdup (tbuff);
  }

  /* macos $HOME/Library/Application Support/BallroomDJ */
  /* this is not the main install dir, but the data directory */
  strlcpy (tbuff, home, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "Library", MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "Application Support", MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "BallroomDJ", MAXPATHLEN);

  if (locationcheck (tbuff)) {
    return strdup (tbuff);
  }

  /* my personal location */
  strlcpy (tbuff, home, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "music-local", MAXPATHLEN);

  if (locationcheck (tbuff)) {
    return strdup (tbuff);
  }

  return "";
}

static bool
locationcheck (char *dir)
{
  char          tbuff [MAXPATHLEN];
  struct stat   statbuf;
  int           rc;

  if (dir == NULL) {
    return false;
  }

  strlcpy (tbuff, dir, MAXPATHLEN);

  rc = stat (tbuff, &statbuf);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "try: %d %s\n", rc, tbuff);
  if (rc == 0) {
    if ((statbuf.st_mode & S_IFDIR) == S_IFDIR) {
      if (locatedb (tbuff)) {
        return true;
      }
    }
  }

  return false;
}


static bool
locatedb (char *dir)
{
  char          tbuff [MAXPATHLEN];
  struct stat   statbuf;
  int           rc;

  strlcpy (tbuff, dir, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "data", MAXPATHLEN);

  rc = stat (tbuff, &statbuf);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "try: %d %s\n", rc, tbuff);
  if (rc != 0) {
    return false;
  }
  if ((statbuf.st_mode & S_IFDIR) != S_IFDIR) {
    return false;
  }

  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "musicdb.txt", MAXPATHLEN);

  rc = stat (tbuff, &statbuf);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "try: %d %s\n", rc, tbuff);
  if (rc != 0) {
    return false;
  }
  if ((statbuf.st_mode & S_IFREG) != S_IFREG) {
    return false;
  }

  return true;
}
