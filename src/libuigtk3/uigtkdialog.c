#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdj4intl.h"
#include "log.h"
#include "ui.h"
#include "uiutils.h"

char *
uiSelectDirDialog (uiselect_t *selectdata)
{
  GtkFileChooserNative  *widget = NULL;
  gint                  res;
  char                  *fn = NULL;

  widget = gtk_file_chooser_native_new (
      selectdata->label,
      GTK_WINDOW (selectdata->window),
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
  GtkFileChooserNative  *widget = NULL;
  gint                  res;
  char                  *fn = NULL;

  widget = gtk_file_chooser_native_new (
      selectdata->label,
      GTK_WINDOW (selectdata->window),
      GTK_FILE_CHOOSER_ACTION_OPEN,
      /* CONTEXT: actions associated with the select file dialog */
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

