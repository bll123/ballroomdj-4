/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uilink.h"

static gboolean uiLinkCallback (GtkLinkButton *lb, gpointer udata);

uiwcont_t *
uiCreateLink (const char *label, const char *uri)
{
  uiwcont_t *uiwidget;
  GtkWidget *widget;
  GtkWidget *lwidget;

  widget = gtk_link_button_new ("");
  if (uri != NULL) {
    gtk_link_button_set_uri (GTK_LINK_BUTTON (widget), uri);
  }
  if (label != NULL) {
    gtk_button_set_label (GTK_BUTTON (widget), label);
  }
  lwidget = gtk_bin_get_child (GTK_BIN (widget));
  gtk_label_set_xalign (GTK_LABEL (lwidget), 0.0);
  gtk_label_set_track_visited_links (GTK_LABEL (lwidget), FALSE);

  uiwidget = uiwcontAlloc (WCONT_T_LINK, WCONT_T_LINK);
  uiwcontSetWidget (uiwidget, widget, NULL);
//  uiwidget->uidata.widget = widget;
//  uiwidget->uidata.packwidget = widget;
  return uiwidget;
}

void
uiLinkSet (uiwcont_t *uilink, const char *label, const char *uri)
{
  if (! uiwcontValid (uilink, WCONT_T_LINK, "link-set")) {
    return;
  }

  if (uri != NULL) {
    gtk_link_button_set_uri (GTK_LINK_BUTTON (uilink->uidata.widget), uri);
  }
  if (label != NULL) {
    gtk_button_set_label (GTK_BUTTON (uilink->uidata.widget), label);
  }
}

void
uiLinkSetActivateCallback (uiwcont_t *uilink, callback_t *uicb)
{
  if (! uiwcontValid (uilink, WCONT_T_LINK, "link-set-cb")) {
    return;
  }

  g_signal_connect (uilink->uidata.widget, "activate-link",
      G_CALLBACK (uiLinkCallback), uicb);
}

static gboolean
uiLinkCallback (GtkLinkButton *lb, gpointer udata)
{
  callback_t *uicb = udata;

  if (! callbackIsSet (uicb)) {
    return FALSE;
  }

  return callbackHandler (uicb);
}
