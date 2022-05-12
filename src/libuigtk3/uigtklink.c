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

void
uiCreateLink (UIWidget *uiwidget, char *label, char *uri)
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
  uiwidget->widget = widget;
}

void
uiLinkSet (UIWidget *uilink, char *label, char *uri)
{
  if (uri != NULL) {
    gtk_link_button_set_uri (GTK_LINK_BUTTON (uilink->widget), uri);
  }
  if (label != NULL) {
    gtk_button_set_label (GTK_BUTTON (uilink->widget), label);
  }
}
