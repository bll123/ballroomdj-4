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
uiCreateSwitch (int value)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiCreateSwitch");

  widget = gtk_switch_new ();
  assert (widget != NULL);
  gtk_switch_set_active (GTK_SWITCH (widget), value);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
  logProcEnd (LOG_PROC, "uiCreateSwitch", "");
  return widget;
}

void
uiSwitchSetValue (GtkWidget *w, int value)
{
  if (w == NULL) {
    return;
  }
  gtk_switch_set_active (GTK_SWITCH (w), value);
}
