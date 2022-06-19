#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

#include <gtk/gtk.h>

#include "bdj4intl.h"
#include "ui.h"

typedef struct uiselect {
  UIWidget    *window;
  const char  *label;
  const char  *startpath;
  const char  *mimefiltername;
  const char  *mimetype;
} uiselect_t;

static void uiDialogResponseHandler (GtkDialog *d, gint responseid, gpointer udata);

char *
uiSelectDirDialog (uiselect_t *selectdata)
{
  GtkFileChooserNative *widget = NULL;
  gint      res;
  char      *fn = NULL;

  widget = gtk_file_chooser_native_new (
      selectdata->label,
      GTK_WINDOW (selectdata->window->widget),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
      /* CONTEXT: actions associated with the select folder dialog */
      _("Select"), _("Close"));

  if (selectdata->startpath != NULL) {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
        selectdata->startpath);
  }

  res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (widget));
  if (res == GTK_RESPONSE_ACCEPT) {
    fn = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
  }

  g_object_unref (widget);
  return fn;
}

char *
uiSelectFileDialog (uiselect_t *selectdata)
{
  GtkFileChooserNative *widget = NULL;
  gint      res;
  char      *fn = NULL;

  widget = gtk_file_chooser_native_new (
      selectdata->label,
      GTK_WINDOW (selectdata->window->widget),
      GTK_FILE_CHOOSER_ACTION_OPEN,
      /* CONTEXT: actions associated with the select folder dialog */
      _("Select"), _("Close"));

  if (selectdata->startpath != NULL) {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
        selectdata->startpath);
  }
  if (selectdata->mimetype != NULL) {
    GtkFileFilter   *ff;

    ff = gtk_file_filter_new ();
    gtk_file_filter_add_mime_type (ff, selectdata->mimetype);
    if (selectdata->mimefiltername != NULL) {
      gtk_file_filter_set_name (ff, selectdata->mimefiltername);
    }
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (widget), ff);
  }

  res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (widget));
  if (res == GTK_RESPONSE_ACCEPT) {
    fn = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
  }

  g_object_unref (widget);
  return fn;
}

void
uiCreateDialog (UIWidget *uiwidget, UIWidget *window,
    UICallback *uicb, const char *title, ...)
{
  GtkWidget *dialog;
  va_list   valist;
  char      *label;
  int       resp;

  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window->widget));
  uiwidget->widget = dialog;

  va_start (valist, title);
  while ((label = va_arg (valist, char *)) != NULL) {
    resp = va_arg (valist, int);
    gtk_dialog_add_button (GTK_DIALOG (dialog), label, resp);
  }
  va_end (valist);

  if (uicb != NULL) {
    g_signal_connect (dialog, "response",
        G_CALLBACK (uiDialogResponseHandler), uicb);
  }
}

void
uiDialogPackInDialog (UIWidget *uidialog, UIWidget *boxp)
{
  GtkWidget *content;

  content = gtk_dialog_get_content_area (GTK_DIALOG (uidialog->widget));
  gtk_container_add (GTK_CONTAINER (content), boxp->widget);
}

void
uiDialogDestroy (UIWidget *uidialog)
{
  if (uidialog->widget == NULL) {
    return;
  }

  if (GTK_IS_WIDGET (uidialog->widget)) {
    gtk_widget_destroy (GTK_WIDGET (uidialog->widget));
  }
  uidialog->widget = NULL;
}

uiselect_t *
uiDialogCreateSelect (UIWidget *window, const char *label,
    const char *startpath, const char *mimefiltername, const char *mimetype)
{
  uiselect_t  *selectdata;

  selectdata = malloc (sizeof (uiselect_t));
  selectdata->window = window;
  selectdata->label = label;
  selectdata->startpath = startpath;
  selectdata->mimefiltername = mimefiltername;
  selectdata->mimetype = mimetype;
  return selectdata;
}


/* internal routines */

static void
uiDialogResponseHandler (GtkDialog *d, gint responseid, gpointer udata)
{
  UICallback  *uicb = udata;
  if (responseid == GTK_RESPONSE_DELETE_EVENT) {
    responseid = RESPONSE_DELETE_WIN;
  }
  uiutilsCallbackLongHandler (uicb, responseid);
}
