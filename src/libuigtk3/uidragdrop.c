/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "callback.h"
#include "mdebug.h"
#include "uiwcont.h"

#include "ui-gtk3.h"

#include "ui/uiwcont-int.h"

#include "ui/uidragdrop.h"

static void uiDragDropDestHandler (GtkWidget *w, GdkDragContext *context,
    gint x, gint y, GtkSelectionData *seldata, guint info, guint time,
    gpointer udata);

void
uiDragDropSetDestURICallback (uiwcont_t *uiwidget, callback_t *cb)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_drag_dest_set (uiwidget->uidata.widget, GTK_DEST_DEFAULT_ALL,
      NULL, 0, GDK_ACTION_COPY);
  gtk_drag_dest_add_uri_targets (uiwidget->uidata.widget);
  gtk_drag_dest_add_text_targets (uiwidget->uidata.widget);
  g_signal_connect (GTK_WIDGET (uiwidget->uidata.widget), "drag-data-received",
      G_CALLBACK (uiDragDropDestHandler), cb);
}


/* internal routines */

static void
uiDragDropDestHandler (GtkWidget *w, GdkDragContext *context,
    gint x, gint y, GtkSelectionData *seldata, guint info, guint tm,
    gpointer udata)
{
  callback_t    *cb = udata;

  /* what are the possible formats? why is there no define for it? */
  if (info == 0 &&
      gtk_selection_data_get_length (seldata) >= 0 &&
      gtk_selection_data_get_format (seldata) == 8) {
    int           rc;
    char          *nstr;
    const char    *data;

    data = (const char *) gtk_selection_data_get_data (seldata);

    nstr = g_uri_unescape_string (data, NULL);
    mdextalloc (nstr);
    rc = callbackHandlerS (cb, nstr);
    mdextfree (nstr);
    free (nstr);
    gtk_drag_finish (context, rc, FALSE, tm);
  }

  if (info == 1 && cb != NULL &&
      gtk_selection_data_get_length (seldata) == sizeof (int32_t) &&
      gtk_selection_data_get_format (seldata) == 32 ) {
    int     rc = UICB_CONT;

    gtk_drag_finish (context, rc, FALSE, tm);
  }

  g_signal_stop_emission_by_name (G_OBJECT (w), "drag-data-received");
}

