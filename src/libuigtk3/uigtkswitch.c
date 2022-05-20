#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "ui.h"
#include "uiutils.h"

void
uiCreateSwitch (UIWidget *uiwidget, int value)
{
  GtkWidget   *widget;

  widget = gtk_switch_new ();
  assert (widget != NULL);
  gtk_switch_set_active (GTK_SWITCH (widget), value);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
  uiwidget->widget = widget;
}

void
uiSwitchSetValue (UIWidget *uiwidget, int value)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }
  gtk_switch_set_active (GTK_SWITCH (uiwidget->widget), value);
}

int
uiSwitchGetValue (UIWidget *uiwidget)
{
  if (uiwidget == NULL) {
    return 0;
  }
  if (uiwidget->widget == NULL) {
    return 0;
  }
  return gtk_switch_get_active (GTK_SWITCH (uiwidget->widget));
}

