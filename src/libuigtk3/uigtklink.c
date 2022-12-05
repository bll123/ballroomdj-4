/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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

#include "ui.h"

static gboolean uiLinkCallback (GtkLinkButton *lb, gpointer udata);

void
uiCreateLink (UIWidget *uiwidget, const char *label, const char *uri)
{
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
  uiwidget->widget = widget;
}

void
uiLinkSet (UIWidget *uilink, const char *label, const char *uri)
{
  if (uri != NULL) {
    gtk_link_button_set_uri (GTK_LINK_BUTTON (uilink->widget), uri);
  }
  if (label != NULL) {
    gtk_button_set_label (GTK_BUTTON (uilink->widget), label);
  }
}

void
uiLinkSetActivateCallback (UIWidget *uilink, UICallback *uicb)
{
  g_signal_connect (uilink->widget, "activate-link",
      G_CALLBACK (uiLinkCallback), uicb);
}

static gboolean
uiLinkCallback (GtkLinkButton *lb, gpointer udata)
{
  UICallback *uicb = udata;

  if (uicb == NULL) {
    return FALSE;
  }
  if (uicb->cb == NULL) {
    return FALSE;
  }

  uicb->cb (uicb->udata);
  return TRUE; // stop other link handlers
}
