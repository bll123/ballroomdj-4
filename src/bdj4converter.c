#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "filedata.h"
#include "locatebdj3.c"
#include "log.h"
#include "pathutil.h"
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
  GtkTextBuffer   *dispBuffer;
  GtkWidget       *dispTextView;
  mstime_t        validateTimer;
} converter_t;

#define CONV_TEMP_FILE "tmp/bdj4convout.txt"

static int  converterCreateGui (converter_t *converter, int argc, char *argv []);
static void converterActivate (GApplication *app, gpointer udata);
static int  converterMainLoop (void *udata);
static void converterSelectDirDialog (GtkButton *b, gpointer udata);
static void converterExit (GtkButton *b, gpointer udata);
static void converterValidateDir (converter_t *converter, bool okonly);
static void converterValidateStart (GtkEditable *e, gpointer udata);
static void converterConvert (GtkButton *b, gpointer udata);
static void converterScrollToEnd (GtkWidget *w, GtkAllocation *retAllocSize, gpointer udata);

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
  converter.dispTextView = NULL;
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
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "initial loc: %s", converter.loc);

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
  GtkWidget             *scwidget;
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

  converter->dispBuffer = gtk_text_buffer_new (NULL);
  converter->dispTextView = gtk_text_view_new_with_buffer (converter->dispBuffer);
  gtk_widget_set_size_request (GTK_WIDGET (converter->dispTextView), -1, 400);
  gtk_widget_set_can_focus (converter->dispTextView, FALSE);
  gtk_widget_set_halign (converter->dispTextView, GTK_ALIGN_FILL);
  gtk_widget_set_valign (converter->dispTextView, GTK_ALIGN_START);
  gtk_widget_set_hexpand (GTK_WIDGET (converter->dispTextView), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (converter->dispTextView), TRUE);
  gtk_container_add (GTK_CONTAINER (scwidget), GTK_WIDGET (converter->dispTextView));
  g_signal_connect (converter->dispTextView,
      "size-allocate", G_CALLBACK (converterScrollToEnd), converter);

  /* push the text view to the top */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_vexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), TRUE, TRUE, 0);

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
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", converter->loc);
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
  converter_t *converter = udata;
  char        bdjpath [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  char        *cvtscript = "cvtall.sh";
  int         rc;
  FILE        *fh;


  strlcpy (bdjpath, converter->loc, MAXPATHLEN);
  pathNormPath (bdjpath, MAXPATHLEN);

  if (isWindows ()) {
    cvtscript = "cvtall.bat";
  }

  snprintf (tbuff, sizeof (tbuff), "conv/%s \"%s\" > %s",
    cvtscript, bdjpath, CONV_TEMP_FILE);

  logMsg (LOG_INSTALL, LOG_IMPORTANT, "cmd: %s", tbuff);
  rc = system (tbuff);

  if (rc == 0) {
    char    *txt;
    char    *p;
    char    *tokstr = NULL;

    txt = filedataReadAll (CONV_TEMP_FILE);
    if (txt != NULL) {
      gtk_text_buffer_set_text (converter->dispBuffer, txt, -1);
      p = strtok_r (txt, "\r\n", &tokstr);
      while (p != NULL) {
        logMsg (LOG_INSTALL, LOG_IMPORTANT, "%s", p);
        p = strtok_r (NULL, "\r\n", &tokstr);
      }
      free (txt);
    }
  } else {
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "fail: %d %s", errno, strerror (errno));
  }

  return;
}


static void
converterScrollToEnd (GtkWidget *w, GtkAllocation *retAllocSize, gpointer udata)
{
  converter_t *converter = udata;
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter (converter->dispBuffer, &iter);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (converter->dispTextView),
      &iter, 0, false, 0, 0);
}
