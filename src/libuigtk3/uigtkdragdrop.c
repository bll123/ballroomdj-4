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

static void uiDragDropDestHandler (GtkWidget *w, GdkDragContext *context,
    gint x, gint y, GtkSelectionData *seldata, guint info, guint time,
    gpointer udata);

void
uiDragDropSetDestURICallback (uiwcont_t *uiwcont, callback_t *cb)
{
  if (uiwcont == NULL) {
    return;
  }
  if (uiwcont->widget == NULL) {
    return;
  }

  if (GTK_IS_TREE_VIEW (uiwcont->widget)) {
    gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (uiwcont->widget),
        NULL, 0, GDK_ACTION_COPY);
  } else {
    gtk_drag_dest_set (uiwcont->widget, GTK_DEST_DEFAULT_ALL,
        NULL, 0, GDK_ACTION_COPY);
  }
  gtk_drag_dest_add_uri_targets (uiwcont->widget);
  g_signal_connect (GTK_WIDGET (uiwcont->widget), "drag-data-received",
      G_CALLBACK (uiDragDropDestHandler), cb);
}


/* internal routines */

static void
uiDragDropDestHandler (GtkWidget *w, GdkDragContext *context,
    gint x, gint y, GtkSelectionData *seldata, guint info, guint tm,
    gpointer udata)
{
  callback_t    *cb = udata;
  int           row = -1;

  if (GTK_IS_TREE_VIEW (w)) {
    uiwcont_t   uiwcont;

    uiwcont.widget = w;
    row = uiTreeViewGetDragDropRow (&uiwcont, x, y);
  }

  /* what are the possible formats? why is there no define for it? */
  if (info == 0 &&
      gtk_selection_data_get_length (seldata) >= 0 &&
      gtk_selection_data_get_format (seldata) == 8) {
    char                **urilist = NULL;
    int                 rc;

    urilist = gtk_selection_data_get_uris (seldata);
    mdextalloc (urilist);

    rc = callbackHandlerStrInt (cb, urilist [0], row);
    mdextfree (urilist);
    g_strfreev (urilist);
    gtk_drag_finish (context, rc, FALSE, tm);
  }

  if (info == 1 &&
      gtk_selection_data_get_length (seldata) == sizeof (int32_t) &&
      gtk_selection_data_get_format (seldata) == 32 ) {
    int     rc;

    rc = callbackHandlerLong (cb, row);
    gtk_drag_finish (context, rc, FALSE, tm);
  }

  g_signal_stop_emission_by_name (G_OBJECT (w), "drag-data-received");
}

