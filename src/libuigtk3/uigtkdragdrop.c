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

/* this is not working at this time */
/* I need better examples */
/* I'd like to wrap an event box around another box or somesuch */

void
uiDragDropSetCallback (uiwcont_t *uiwcont, callback_t *cb)
{
  if (uiwcont == NULL) {
    return;
  }
  if (uiwcont->widget == NULL) {
    return;
  }

  gtk_drag_dest_set (uiwcont->widget,
      GTK_DEST_DEFAULT_DROP | GTK_DEST_DEFAULT_HIGHLIGHT,
      NULL, 0, GDK_ACTION_ASK);
  gtk_drag_dest_add_uri_targets (uiwcont->widget);
  g_signal_connect (uiwcont->widget, "drag-drop-received",
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
    GdkDragAction action;

    action = gdk_drag_context_get_selected_action (context);
    if (action == GDK_ACTION_ASK) {
      int   rc;
      char  **uri = NULL;

      uri = gtk_selection_data_get_uris (seldata);
      mdextalloc (uri);
      rc = callbackHandlerStr (cb, uri [0]);
      mdextfree (uri);
      g_strfreev (uri);
#if 0
      if (response == GTK_RESPONSE_YES) {
        action = GDK_ACTION_MOVE;
      } else {
        action = GDK_ACTION_COPY;
      }
#endif
      gtk_drag_finish (context, TRUE, FALSE, tm);
    } else {
      gtk_drag_finish (context, FALSE, FALSE, tm);
    }
  }
  /* is gtk_drag_finish required for other data formats? */
}

