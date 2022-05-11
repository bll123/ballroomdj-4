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

#include "log.h"
#include "ui.h"
#include "uiutils.h"

GtkWidget *
uiCreateLink (char *label, char *uri)
{
  GtkWidget *widget;
  GtkWidget *lwidget;

  widget = gtk_link_button_new ("");
  gtk_link_button_set_uri (GTK_LINK_BUTTON (widget), uri);
  gtk_button_set_label (GTK_BUTTON (widget), label);
  lwidget = gtk_bin_get_child (GTK_BIN (widget));
  gtk_label_set_xalign (GTK_LABEL (lwidget), 0.0);
  return widget;
}

void
uiLinkSet (GtkWidget *widget, char *label, char *uri)
{
  gtk_link_button_set_uri (GTK_LINK_BUTTON (widget), uri);
  gtk_button_set_label (GTK_BUTTON (widget), label);
}
