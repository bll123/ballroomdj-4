/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "callback.h"
#include "mdebug.h"
#include "uiwcont.h"

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "ui/uiwcont-int.h"

#include "ui/uidragdrop.h"

static void uiDragDropSignalHandler (GtkWidget *w, GdkDragContext *context,
    gint x, gint y, GtkSelectionData *seldata, guint info, guint time,
    gpointer udata);

void
uiDragDropSetCallback (uiwcont_t *uiwcont, callback_t *cb)
{
  static GtkTargetEntry targetentries[] = {
    { "text/uri-list", 0, 0}
  };

  if (uiwcont == NULL) {
    return;
  }
  if (uiwcont->widget == NULL) {
    return;
  }

  gtk_drag_dest_set (uiwcont->widget, GTK_DEST_DEFAULT_ALL,
      targetentries, 1, GDK_ACTION_COPY);
//  gtk_drag_dest_add_uri_targets (uiwcont->widget);
  g_signal_connect (GTK_WIDGET (uiwcont->widget), "drag-data-received",
      G_CALLBACK (uiDragDropSignalHandler), cb);
}

/* internal routines */

static void
uiDragDropSignalHandler (GtkWidget *w, GdkDragContext *context,
    gint x, gint y, GtkSelectionData *seldata, guint info, guint tm,
    gpointer udata)
{
  callback_t    *cb = udata;

fprintf (stderr, "info: %d\n", info);
fprintf (stderr, "length: %d\n", gtk_selection_data_get_length (seldata));
fprintf (stderr, "format: %d\n", gtk_selection_data_get_format (seldata));
  /* what are the possible formats? why is there no define for it? */
  if (gtk_selection_data_get_length (seldata) >= 0 &&
      gtk_selection_data_get_format (seldata) == 8) {
    char  **urilist = NULL;
    int   rc;

    urilist = gtk_selection_data_get_uris (seldata);
    mdextalloc (urilist);

    rc = callbackHandlerStr (cb, urilist [0]);
    mdextfree (urilist);
    g_strfreev (urilist);
    gtk_drag_finish (context, rc, FALSE, tm);
  }

  g_signal_stop_emission_by_name (G_OBJECT (w), "drag-data-received");
}

