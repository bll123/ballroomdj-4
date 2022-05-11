#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <gtk/gtk.h>

#include "log.h"
#include "ui.h"
#include "uiutils.h"

GtkWidget *
uiCreateCheckButton (const char *txt, int value)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiCreateCheckButton");

  widget = gtk_check_button_new_with_label (txt);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  logProcEnd (LOG_PROC, "uiCreateCheckButton", "");
  return widget;
}

bool
uiToggleButtonIsActive (GtkWidget *widget)
{
  return (gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (widget)));
}
